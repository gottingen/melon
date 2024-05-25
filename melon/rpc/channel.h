//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//

#pragma once

#include <ostream>                          // std::ostream
#include <melon/fiber/errno.h>                  // Redefine errno
#include <melon/utility/intrusive_ptr.hpp>          // mutil::intrusive_ptr
#include <melon/utility/ptr_container.h>
#include <melon/rpc/ssl_options.h>               // ChannelSSLOptions
#include <melon/rpc/channel_base.h>              // ChannelBase
#include <melon/rpc/adaptive_protocol_type.h>    // AdaptiveProtocolType
#include <melon/rpc/adaptive_connection_type.h>  // AdaptiveConnectionType
#include <melon/rpc/socket_id.h>                 // SocketId
#include <melon/rpc/controller.h>                // melon::Controller
#include <melon/rpc/details/profiler_linker.h>
#include <melon/rpc/retry_policy.h>
#include <melon/naming/naming_service_filter.h>

namespace melon {

    struct ChannelOptions {
        // Constructed with default options.
        ChannelOptions();

        // Issue error when a connection is not established after so many
        // milliseconds. -1 means wait indefinitely.
        // Default: 200 (milliseconds)
        // Maximum: 0x7fffffff (roughly 30 days)
        int32_t connect_timeout_ms;

        // Max duration of RPC over this Channel. -1 means wait indefinitely.
        // Overridable by Controller.set_timeout_ms().
        // Default: 500 (milliseconds)
        // Maximum: 0x7fffffff (roughly 30 days)
        int32_t timeout_ms;

        // Send another request if RPC does not finish after so many milliseconds.
        // Overridable by Controller.set_backup_request_ms().
        // The request will be sent to a different server by best effort.
        // If timeout_ms is set and backup_request_ms >= timeout_ms, backup request
        // will never be sent.
        // backup request does NOT imply server-side cancelation.
        // Default: -1 (disabled)
        // Maximum: 0x7fffffff (roughly 30 days)
        int32_t backup_request_ms;

        // Retry limit for RPC over this Channel. <=0 means no retry.
        // Overridable by Controller.set_max_retry().
        // Default: 3
        // Maximum: INT_MAX
        int max_retry;

        // When the error rate of a server node is too high, isolate the node.
        // Note that this isolation is GLOBAL, the node will become unavailable
        // for all channels running in this process during the isolation.
        // Default: false
        bool enable_circuit_breaker;

        // Serialization protocol, defined in melon/proto/options.proto
        // NOTE: You can assign name of the protocol to this field as well, for
        // Example: options.protocol = "melon_std";
        AdaptiveProtocolType protocol;

        // Type of connection to server. If unset, use the default connection type
        // of the protocol.
        // NOTE: You can assign name of the type to this field as well, for
        // Example: options.connection_type = "single";
        // Possible values: "single", "pooled", "short".
        AdaptiveConnectionType connection_type;

        // Channel.Init() succeeds even if there's no server in the NamingService.
        // E.g. the BNS directory is empty. All RPC over the channel will fail before
        // new nodes being added to the NamingService.
        // Default: true (false before r32470)
        bool succeed_without_server;
        // Print a log when above situation happens.
        // Default: true.
        bool log_succeed_without_server;

        // SSL related options. Refer to `ChannelSSLOptions' for details
        bool has_ssl_options() const { return _ssl_options != NULL; }

        const ChannelSSLOptions &ssl_options() const { return *_ssl_options; }

        ChannelSSLOptions *mutable_ssl_options();

        // Let this channel use rdma rather than tcp.
        // Default: false
        bool use_rdma;

        // Turn on authentication for this channel if `auth' is not NULL.
        // Note `auth' will not be deleted by channel and must remain valid when
        // the channel is being used.
        // Default: NULL
        const Authenticator *auth;

        // Customize the error code that should be retried. The interface is
        // defined in melon/retry_policy.h
        // This object is NOT owned by channel and should remain valid when
        // channel is used.
        // Default: NULL
        const RetryPolicy *retry_policy;

        // Filter ServerNodes (i.e. based on `tag' field of `ServerNode')
        // which are generated by NamingService. The interface is defined
        // in melon/naming_service_filter.h
        // This object is NOT owned by channel and should remain valid when
        // channel is used.
        // Default: NULL
        const NamingServiceFilter *ns_filter;

        // Channels with same connection_group share connections.
        // In other words, set to a different value to stop sharing connections.
        // Case-sensitive, leading and trailing spaces are ignored.
        // Default: ""
        std::string connection_group;

    private:
        // SSLOptions is large and not often used, allocate it on heap to
        // prevent ChannelOptions from being bloated in most cases.
        mutil::PtrContainer<ChannelSSLOptions> _ssl_options;
    };

// A Channel represents a communication line to one server or multiple servers
// which can be used to call that Server's services. Servers may be running
// on another machines. Normally, you should not call a Channel directly, but
// instead construct a stub Service wrapping it.
// Example:
//   melon::Channel channel;
//   channel.Init("bns://rdev.matrix.all", "rr", NULL/*default options*/);
//   MyService_Stub stub(&channel);
//   stub.MyMethod(&controller, &request, &response, NULL);
    class Channel : public ChannelBase {
        friend class Controller;

        friend class SelectiveChannel;

    public:
        Channel(ProfilerLinker = ProfilerLinker());

        virtual ~Channel();

        DISALLOW_COPY_AND_ASSIGN(Channel);

        // Connect this channel to a single server whose address is given by the
        // first parameter. Use default options if `options' is NULL.
        int Init(mutil::EndPoint server_addr_and_port, const ChannelOptions *options);

        int Init(const char *server_addr_and_port, const ChannelOptions *options);

        int Init(const char *server_addr, int port, const ChannelOptions *options);

        // Connect this channel to a group of servers whose addresses can be
        // accessed via `naming_service_url' according to its protocol. Use the
        // method specified by `load_balancer_name' to distribute traffic to
        // servers. Use default options if `options' is NULL.
        // Supported naming service("protocol://service_name"):
        //   bns://<node-name>            # Baidu Naming Service
        //   file://<file-path>           # load addresses from the file
        //   list://addr1,addr2,...       # use the addresses separated by comma
        //   http://<url>                 # Domain Naming Service, aka DNS.
        // Supported load balancer:
        //   rr                           # round robin, choose next server
        //   random                       # randomly choose a server
        //   wr                           # weighted random
        //   wrr                          # weighted round robin
        //   la                           # locality aware
        //   c_murmurhash/c_md5           # consistent hashing with murmurhash3/md5
        //   "" or NULL                   # treat `naming_service_url' as `server_addr_and_port'
        //                                # Init(xxx, "", options) and Init(xxx, NULL, options)
        //                                # are exactly same with Init(xxx, options)
        int Init(const char *naming_service_url,
                 const char *load_balancer_name,
                 const ChannelOptions *options);

        // Call `method' of the remote service with `request' as input, and
        // `response' as output. `controller' contains options and extra data.
        // If `done' is not NULL, this method returns after request was sent
        // and `done->Run()' will be called when the call finishes, otherwise
        // caller blocks until the call finishes.
        void CallMethod(const google::protobuf::MethodDescriptor *method,
                        google::protobuf::RpcController *controller,
                        const google::protobuf::Message *request,
                        google::protobuf::Message *response,
                        google::protobuf::Closure *done);

        // Get current options.
        const ChannelOptions &options() const { return _options; }

        void Describe(std::ostream &, const DescribeOptions &) const;

        // Sum of weights of servers that this channel connects to.
        int Weight();

    protected:
        int CheckHealth();

        bool SingleServer() const { return _lb.get() == NULL; }

        // Pick a server using `lb' and then send RPC. Wait for response when
        // sending synchronous RPC.
        // NOTE: DO NOT directly use `controller' after this call when
        // sending asynchronous RPC (controller->_done != NULL) since
        // user callback `done' could be called when it returns and
        // therefore destroy the `controller' inside `done'
        static void CallMethodImpl(Controller *controller, SharedLoadBalancer *lb);

        int InitChannelOptions(const ChannelOptions *options);

        int InitSingle(const mutil::EndPoint &server_addr_and_port,
                       const char *raw_server_address,
                       const ChannelOptions *options,
                       int raw_port = -1);

        std::string _service_name;
        std::string _scheme;
        mutil::EndPoint _server_address;
        SocketId _server_id;
        Protocol::SerializeRequest _serialize_request;
        Protocol::PackRequest _pack_request;
        Protocol::GetMethodName _get_method_name;
        // This will be shared between channel and controllers that
        // are in the middle of RPC procedure using this channel.
        // It will be destroyed after channel's destruction and all
        // the RPC above has finished
        mutil::intrusive_ptr<SharedLoadBalancer> _lb;
        ChannelOptions _options;
        int _preferred_index;
    };

    enum ChannelOwnership {
        OWNS_CHANNEL,
        DOESNT_OWN_CHANNEL
    };

} // namespace melon
