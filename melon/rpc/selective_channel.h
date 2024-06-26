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



#ifndef MELON_RPC_SELECTIVE_CHANNEL_H_
#define MELON_RPC_SELECTIVE_CHANNEL_H_



#include <melon/rpc/socket_id.h>
#include <melon/rpc/channel.h>


namespace melon {

// A combo channel to split traffic to sub channels, aka "schan". The main
// purpose of schan is to load balance between groups of servers.
// SelectiveChannel is a fully functional Channel:
//   * synchronous and asynchronous RPC.
//   * deletable immediately after an asynchronous call.
//   * cancelable call_id (cancels all sub calls).
//   * timeout.
// Due to its designing goal, schan has a separate layer of retrying and
// backup request. Namely when schan fails to access a sub channel, it may
// retry another channel. sub channels inside a schan share the information
// of accessed servers and avoid retrying accessed servers by best efforts.
// When a schan would send a backup request, it calls a sub channel with
// the request. Since a sub channel can be a combo channel as well, the
// "backup request" may be "backup requests".
//                                        ^
// CAUTION:
// =======
// Currently SelectiveChannel requires `request' to CallMethod be
// valid before the RPC ends. Other channels do not. If you're doing async
// calls with SelectiveChannel, make sure that `request' is owned and deleted
// in `done'.
class SelectiveChannel : public ChannelBase/*non-copyable*/ {
public:
    typedef SocketId ChannelHandle;

    SelectiveChannel();
    ~SelectiveChannel();

    // You MUST initialize a schan before using it. `load_balancer_name' is the
    // name of load balancing algorithm which is listed in melon/rpc/channel.h
    // if `options' is NULL, use default options.
    int Init(const char* load_balancer_name, const ChannelOptions* options);

    // Add a sub channel, which will be deleted along with schan or explicitly
    // by RemoveAndDestroyChannel.
    // On success, handle is set with the key for removal.
    // NOTE: Different from pchan, schan can add channels at any time.
    // Returns 0 on success, -1 otherwise.
    int AddChannel(ChannelBase* sub_channel, ChannelHandle* handle);

    // Remove and destroy the sub_channel associated with `handle'.
    void RemoveAndDestroyChannel(ChannelHandle handle);

    // Send request by a sub channel. schan may retry another sub channel
    // according to retrying/backup-request settings.
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done);

    // True iff Init() was successful.
    bool initialized() const;

    void Describe(std::ostream& os, const DescribeOptions& options) const;

private:
    int CheckHealth();
    
    Channel _chan;
};

} // namespace melon


#endif  // MELON_RPC_SELECTIVE_CHANNEL_H_
