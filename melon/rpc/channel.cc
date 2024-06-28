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



#include <inttypes.h>
#include <google/protobuf/descriptor.h>
#include <memory>
#include <melon/utility/time.h>                              // milliseconds_from_now
#include <turbo/log/logging.h>
#include <melon/utility/third_party/murmurhash3/murmurhash3.h>
#include <melon/utility/strings/string_util.h>
#include <melon/fiber/unstable.h>                        // fiber_timer_add
#include <melon/rpc/socket_map.h>                         // SocketMapInsert
#include <melon/rpc/compress.h>
#include <melon/rpc/global.h>
#include <melon/rpc/span.h>
#include <melon/rpc/details/load_balancer_with_naming.h>
#include <melon/rpc/controller.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/details/usercode_backup_pool.h>       // TooManyUserCode
#include <turbo/flags/declare.h>
TURBO_DECLARE_FLAG(bool, enable_rpcz);
TURBO_DECLARE_FLAG(bool, usercode_in_pthread);
namespace melon {

ChannelOptions::ChannelOptions()
    : connect_timeout_ms(200)
    , timeout_ms(500)
    , backup_request_ms(-1)
    , max_retry(3)
    , enable_circuit_breaker(false)
    , protocol(PROTOCOL_MELON_STD)
    , connection_type(CONNECTION_TYPE_UNKNOWN)
    , succeed_without_server(true)
    , log_succeed_without_server(true)
    , use_rdma(false)
    , auth(nullptr)
    , retry_policy(nullptr)
    , ns_filter(nullptr)
{}

ChannelSSLOptions* ChannelOptions::mutable_ssl_options() {
    if (!_ssl_options) {
        _ssl_options.reset(new ChannelSSLOptions);
    }
    return _ssl_options.get();
}

static ChannelSignature ComputeChannelSignature(const ChannelOptions& opt) {
    if (opt.auth == nullptr &&
        !opt.has_ssl_options() &&
        opt.connection_group.empty()) {
        // Returning zeroized result by default is more intuitive for users.
        return ChannelSignature();
    }
    uint32_t seed = 0;
    std::string buf;
    buf.reserve(1024);
    mutil::MurmurHash3_x64_128_Context mm_ctx;
    do {
        buf.clear();
        mutil::MurmurHash3_x64_128_Init(&mm_ctx, seed);

        if (!opt.connection_group.empty()) {
            buf.append("|conng=");
            buf.append(opt.connection_group);
        }
        if (opt.auth) {
            buf.append("|auth=");
            buf.append((char*)&opt.auth, sizeof(opt.auth));
        }
        if (opt.has_ssl_options()) {
            const ChannelSSLOptions& ssl = opt.ssl_options();
            buf.push_back('|');
            buf.append(ssl.ciphers);
            buf.push_back('|');
            buf.append(ssl.protocols);
            buf.push_back('|');
            buf.append(ssl.sni_name);
            const VerifyOptions& verify = ssl.verify;
            buf.push_back('|');
            buf.append((char*)&verify.verify_depth, sizeof(verify.verify_depth));
            buf.push_back('|');
            buf.append(verify.ca_file_path);
        } else {
            // All disabled ChannelSSLOptions are the same
        }
        if (opt.use_rdma) {
            buf.append("|rdma");
        }
        mutil::MurmurHash3_x64_128_Update(&mm_ctx, buf.data(), buf.size());
        buf.clear();
    
        if (opt.has_ssl_options()) {
            const CertInfo& cert = opt.ssl_options().client_cert;
            if (!cert.certificate.empty()) {
                // Certificate may be too long (PEM string) to fit into `buf'
                mutil::MurmurHash3_x64_128_Update(
                    &mm_ctx, cert.certificate.data(), cert.certificate.size());
                mutil::MurmurHash3_x64_128_Update(
                    &mm_ctx, cert.private_key.data(), cert.private_key.size());
            }
        }
        // sni_filters has no effect in ChannelSSLOptions
        ChannelSignature result;
        mutil::MurmurHash3_x64_128_Final(result.data, &mm_ctx);
        if (result != ChannelSignature()) {
            // the empty result is reserved for default case and cannot
            // be used, increment the seed and retry.
            return result;
        }
        ++seed;
    } while (true);
}

Channel::Channel(ProfilerLinker)
    : _server_id(INVALID_SOCKET_ID)
    , _serialize_request(nullptr)
    , _pack_request(nullptr)
    , _get_method_name(nullptr)
    , _preferred_index(-1) {
}

Channel::~Channel() {
    if (_server_id != INVALID_SOCKET_ID) {
        const ChannelSignature sig = ComputeChannelSignature(_options);
        SocketMapRemove(SocketMapKey(_server_address, sig));
    }
}

int Channel::InitChannelOptions(const ChannelOptions* options) {
    if (options) {  // Override default options if user provided one.
        _options = *options;
    }
    const Protocol* protocol = FindProtocol(_options.protocol);
    if (nullptr == protocol || !protocol->support_client()) {
        LOG(ERROR) << "Channel does not support the protocol";
        return -1;
    }

    if (_options.use_rdma) {
        LOG(WARNING) << "Cannot use rdma since melon does not compile with rdma";
        return -1;
    }

    _serialize_request = protocol->serialize_request;
    _pack_request = protocol->pack_request;
    _get_method_name = protocol->get_method_name;

    // Check connection_type
    if (_options.connection_type == CONNECTION_TYPE_UNKNOWN) {
        // Save has_error which will be overriden in later assignments to
        // connection_type.
        const bool has_error = _options.connection_type.has_error();
        
        if (protocol->supported_connection_type & CONNECTION_TYPE_SINGLE) {
            _options.connection_type = CONNECTION_TYPE_SINGLE;
        } else if (protocol->supported_connection_type & CONNECTION_TYPE_POOLED) {
            _options.connection_type = CONNECTION_TYPE_POOLED;
        } else {
            _options.connection_type = CONNECTION_TYPE_SHORT;
        }
        if (has_error) {
            LOG(ERROR) << "Channel=" << this << " chose connection_type="
                       << _options.connection_type.name() << " for protocol="
                       << _options.protocol.name();
        }
    } else {
        if (!(_options.connection_type & protocol->supported_connection_type)) {
            LOG(ERROR) << protocol->name << " does not support connection_type="
                       << ConnectionTypeToString(_options.connection_type);
            return -1;
        }
    }

    _preferred_index = get_client_side_messenger()->FindProtocolIndex(_options.protocol);
    if (_preferred_index < 0) {
        LOG(ERROR) << "Fail to get index for protocol="
                   << _options.protocol.name();
        return -1;
    }

    // Normalize connection_group
    std::string& cg = _options.connection_group;
    if (!cg.empty() && (::isspace(cg.front()) || ::isspace(cg.back()))) {
        mutil::TrimWhitespace(cg, mutil::TRIM_ALL, &cg);
    }
    return 0;
}

int Channel::Init(const char* server_addr_and_port,
                  const ChannelOptions* options) {
    GlobalInitializeOrDie();
    mutil::EndPoint point;
    const AdaptiveProtocolType& ptype = (options ? options->protocol : _options.protocol);
    const Protocol* protocol = FindProtocol(ptype);
    if (protocol == nullptr || !protocol->support_client()) {
        LOG(ERROR) << "Channel does not support the protocol";
        return -1;
    }
    if (protocol->parse_server_address != nullptr) {
        if (!protocol->parse_server_address(&point, server_addr_and_port)) {
            LOG(ERROR) << "Fail to parse address=`" << server_addr_and_port << '\'';
            return -1;
        }
    } else {
        if (str2endpoint(server_addr_and_port, &point) != 0 &&
            hostname2endpoint(server_addr_and_port, &point) != 0) {
            // Many users called the wrong Init(). Print some log to save
            // our troubleshooting time.
            if (strstr(server_addr_and_port, "://")) {
                LOG(ERROR) << "Invalid address=`" << server_addr_and_port
                           << "'. Use Init(naming_service_name, "
                    "load_balancer_name, options) instead.";
            } else {
                LOG(ERROR) << "Invalid address=`" << server_addr_and_port << '\'';
            }
            return -1;
        }
    }
    return InitSingle(point, server_addr_and_port, options);
}

int Channel::Init(const char* server_addr, int port,
                  const ChannelOptions* options) {
    GlobalInitializeOrDie();
    mutil::EndPoint point;
    const AdaptiveProtocolType& ptype = (options ? options->protocol : _options.protocol);
    const Protocol* protocol = FindProtocol(ptype);
    if (protocol == nullptr || !protocol->support_client()) {
        LOG(ERROR) << "Channel does not support the protocol";
        return -1;
    }
    if (protocol->parse_server_address != nullptr) {
        if (!protocol->parse_server_address(&point, server_addr)) {
            LOG(ERROR) << "Fail to parse address=`" << server_addr << '\'';
            return -1;
        }
        point.port = port;
    } else {
        if (str2endpoint(server_addr, port, &point) != 0 &&
            hostname2endpoint(server_addr, port, &point) != 0) {
            LOG(ERROR) << "Invalid address=`" << server_addr << '\'';
            return -1;
        }
    }
    return InitSingle(point, server_addr, options, port);
}

static int CreateSocketSSLContext(const ChannelOptions& options,
                                  std::shared_ptr<SocketSSLContext>* ssl_ctx) {
    if (options.has_ssl_options()) {
        SSL_CTX* raw_ctx = CreateClientSSLContext(options.ssl_options());
        if (!raw_ctx) {
            LOG(ERROR) << "Fail to CreateClientSSLContext";
            return -1;
        }
        *ssl_ctx = std::make_shared<SocketSSLContext>();
        (*ssl_ctx)->raw_ctx = raw_ctx;
        (*ssl_ctx)->sni_name = options.ssl_options().sni_name;
        (*ssl_ctx)->alpn_protocols = options.ssl_options().alpn_protocols;
    } else {
        (*ssl_ctx) = nullptr;
    }
    return 0;
}

int Channel::Init(mutil::EndPoint server_addr_and_port,
                  const ChannelOptions* options) {
    return InitSingle(server_addr_and_port, "", options);
}

int Channel::InitSingle(const mutil::EndPoint& server_addr_and_port,
                        const char* raw_server_address,
                        const ChannelOptions* options,
                        int raw_port) {
    GlobalInitializeOrDie();
    if (InitChannelOptions(options) != 0) {
        return -1;
    }
    int* port_out = raw_port == -1 ? &raw_port: nullptr;
    ParseURL(raw_server_address, &_scheme, &_service_name, port_out);
    if (raw_port != -1) {
        _service_name.append(":").append(std::to_string(raw_port));
    }
    if (_options.protocol == melon::PROTOCOL_HTTP && _scheme == "https") {
        if (_options.mutable_ssl_options()->sni_name.empty()) {
            _options.mutable_ssl_options()->sni_name = _service_name;
        }
    }
    const int port = server_addr_and_port.port;
    if (port < 0) {
        LOG(ERROR) << "Invalid port=" << port;
        return -1;
    }
    _server_address = server_addr_and_port;
    const ChannelSignature sig = ComputeChannelSignature(_options);
    std::shared_ptr<SocketSSLContext> ssl_ctx;
    if (CreateSocketSSLContext(_options, &ssl_ctx) != 0) {
        return -1;
    }
    if (SocketMapInsert(SocketMapKey(server_addr_and_port, sig),
                        &_server_id, ssl_ctx, _options.use_rdma) != 0) {
        LOG(ERROR) << "Fail to insert into SocketMap";
        return -1;
    }
    return 0;
}

int Channel::Init(const char* ns_url,
                  const char* lb_name,
                  const ChannelOptions* options) {
    if (lb_name == nullptr || *lb_name == '\0') {
        // Treat ns_url as server_addr_and_port
        return Init(ns_url, options);
    }
    GlobalInitializeOrDie();
    if (InitChannelOptions(options) != 0) {
        return -1;
    }
    int raw_port = -1;
    ParseURL(ns_url, &_scheme, &_service_name, &raw_port);
    if (raw_port != -1) {
        _service_name.append(":").append(std::to_string(raw_port));
    }
    if (_options.protocol == melon::PROTOCOL_HTTP && _scheme == "https") {
        if (_options.mutable_ssl_options()->sni_name.empty()) {
            _options.mutable_ssl_options()->sni_name = _service_name;
        }
    }
    std::unique_ptr<LoadBalancerWithNaming> lb(new (std::nothrow)
                                                   LoadBalancerWithNaming);
    if (nullptr == lb) {
        LOG(FATAL) << "Fail to new LoadBalancerWithNaming";
        return -1;        
    }
    GetNamingServiceThreadOptions ns_opt;
    ns_opt.succeed_without_server = _options.succeed_without_server;
    ns_opt.log_succeed_without_server = _options.log_succeed_without_server;
    ns_opt.use_rdma = _options.use_rdma;
    ns_opt.channel_signature = ComputeChannelSignature(_options);
    if (CreateSocketSSLContext(_options, &ns_opt.ssl_ctx) != 0) {
        return -1;
    }
    if (lb->Init(ns_url, lb_name, _options.ns_filter, &ns_opt) != 0) {
        LOG(ERROR) << "Fail to initialize LoadBalancerWithNaming";
        return -1;
    }
    _lb.reset(lb.release());
    return 0;
}

static void HandleTimeout(void* arg) {
    fiber_session_t correlation_id = { (uint64_t)arg };
    fiber_session_error(correlation_id, ERPCTIMEDOUT);
}

static void HandleBackupRequest(void* arg) {
    fiber_session_t correlation_id = { (uint64_t)arg };
    fiber_session_error(correlation_id, EBACKUPREQUEST);
}

void Channel::CallMethod(const google::protobuf::MethodDescriptor* method,
                         google::protobuf::RpcController* controller_base,
                         const google::protobuf::Message* request,
                         google::protobuf::Message* response,
                         google::protobuf::Closure* done) {
    const int64_t start_send_real_us = mutil::gettimeofday_us();
    Controller* cntl = static_cast<Controller*>(controller_base);
    cntl->OnRPCBegin(start_send_real_us);
    // Override max_retry first to reset the range of correlation_id
    if (cntl->max_retry() == UNSET_MAGIC_NUM) {
        cntl->set_max_retry(_options.max_retry);
    }
    if (cntl->max_retry() < 0) {
        // this is important because #max_retry decides #versions allocated
        // in correlation_id. negative max_retry causes undefined behavior.
        cntl->set_max_retry(0);
    }
    // HTTP needs this field to be set before any SetFailed()
    cntl->_request_protocol = _options.protocol;
    if (_options.protocol.has_param()) {
        CHECK(cntl->protocol_param().empty());
        cntl->protocol_param() = _options.protocol.param();
    }
    if (_options.protocol == melon::PROTOCOL_HTTP && (_scheme == "https" || _scheme == "http")) {
        URI& uri = cntl->http_request().uri();
        if (uri.host().empty() && !_service_name.empty()) {
            uri.SetHostAndPort(_service_name);
        }
    }
    cntl->_preferred_index = _preferred_index;
    cntl->_retry_policy = _options.retry_policy;
    if (_options.enable_circuit_breaker) {
        cntl->add_flag(Controller::FLAGS_ENABLED_CIRCUIT_BREAKER);
    }
    const CallId correlation_id = cntl->call_id();
    const int rc = fiber_session_lock_and_reset_range(
                    correlation_id, nullptr, 2 + cntl->max_retry());
    if (rc != 0) {
        CHECK_EQ(EINVAL, rc);
        if (!cntl->FailedInline()) {
            cntl->SetFailed(EINVAL, "Fail to lock call_id=%" PRId64,
                            correlation_id.value);
        }
        LOG_IF(ERROR, cntl->is_used_by_rpc())
            << "Controller=" << cntl << " was used by another RPC before. "
            "Did you forget to Reset() it before reuse?";
        // Have to run done in-place. If the done runs in another thread,
        // Join() on this RPC is no-op and probably ends earlier than running
        // the callback and releases resources used in the callback.
        // Since this branch is only entered by wrongly-used RPC, the
        // potentially introduced deadlock(caused by locking RPC and done with
        // the same non-recursive lock) is acceptable and removable by fixing
        // user's code.
        if (done) {
            done->Run();
        }
        return;
    }
    cntl->set_used_by_rpc();

    if (cntl->_sender == nullptr && IsTraceable(Span::tls_parent())) {
        const int64_t start_send_us = mutil::cpuwide_time_us();
        const std::string* method_name = nullptr;
        if (_get_method_name) {
            method_name = &_get_method_name(method, cntl);
        } else if (method) {
            method_name = &method->full_name();
        } else {
            const static std::string NULL_METHOD_STR = "null-method";
            method_name = &NULL_METHOD_STR;
        }
        Span* span = Span::CreateClientSpan(
            *method_name, start_send_real_us - start_send_us);
        span->set_log_id(cntl->log_id());
        span->set_base_cid(correlation_id);
        span->set_protocol(_options.protocol);
        span->set_start_send_us(start_send_us);
        cntl->_span = span;
    }
    // Override some options if they haven't been set by Controller
    if (cntl->timeout_ms() == UNSET_MAGIC_NUM) {
        cntl->set_timeout_ms(_options.timeout_ms);
    }
    // Since connection is shared extensively amongst channels and RPC,
    // overriding connect_timeout_ms does not make sense, just use the
    // one in ChannelOptions
    cntl->_connect_timeout_ms = _options.connect_timeout_ms;
    if (cntl->backup_request_ms() == UNSET_MAGIC_NUM) {
        cntl->set_backup_request_ms(_options.backup_request_ms);
    }
    if (cntl->connection_type() == CONNECTION_TYPE_UNKNOWN) {
        cntl->set_connection_type(_options.connection_type);
    }
    cntl->_response = response;
    cntl->_done = done;
    cntl->_pack_request = _pack_request;
    cntl->_method = method;
    cntl->_auth = _options.auth;

    if (SingleServer()) {
        cntl->_single_server_id = _server_id;
        cntl->_remote_side = _server_address;
    }

    // Share the lb with controller.
    cntl->_lb = _lb;

    // Ensure that serialize_request is done before pack_request in all
    // possible executions, including:
    //   HandleSendFailed => OnVersionedRPCReturned => IssueRPC(pack_request)
    _serialize_request(&cntl->_request_buf, cntl, request);
    if (cntl->FailedInline()) {
        // Handle failures caused by serialize_request, and these error_codes
        // should be excluded from the retry_policy.
        return cntl->HandleSendFailed();
    }
    if (turbo::get_flag(FLAGS_usercode_in_pthread) &&
        done != nullptr &&
        TooManyUserCode()) {
        cntl->SetFailed(ELIMIT, "Too many user code to run when "
                        "-usercode_in_pthread is on");
        return cntl->HandleSendFailed();
    }

    if (cntl->_request_stream != INVALID_STREAM_ID) {
        // Currently we cannot handle retry and backup request correctly
        cntl->set_max_retry(0);
        cntl->set_backup_request_ms(-1);
    }

    if (cntl->backup_request_ms() >= 0 &&
        (cntl->backup_request_ms() < cntl->timeout_ms() ||
         cntl->timeout_ms() < 0)) {
        // Setup timer for backup request. When it occurs, we'll setup a
        // timer of timeout_ms before sending backup request.

        // _deadline_us is for truncating _connect_timeout_ms and resetting
        // timer when EBACKUPREQUEST occurs.
        if (cntl->timeout_ms() < 0) {
            cntl->_deadline_us = -1;
        } else {
            cntl->_deadline_us = cntl->timeout_ms() * 1000L + start_send_real_us;
        }
        const int rc = fiber_timer_add(
            &cntl->_timeout_id,
            mutil::microseconds_to_timespec(
                cntl->backup_request_ms() * 1000L + start_send_real_us),
            HandleBackupRequest, (void*)correlation_id.value);
        if (MELON_UNLIKELY(rc != 0)) {
            cntl->SetFailed(rc, "Fail to add timer for backup request");
            return cntl->HandleSendFailed();
        }
    } else if (cntl->timeout_ms() >= 0) {
        // Setup timer for RPC timetout

        // _deadline_us is for truncating _connect_timeout_ms
        cntl->_deadline_us = cntl->timeout_ms() * 1000L + start_send_real_us;
        const int rc = fiber_timer_add(
            &cntl->_timeout_id,
            mutil::microseconds_to_timespec(cntl->_deadline_us),
            HandleTimeout, (void*)correlation_id.value);
        if (MELON_UNLIKELY(rc != 0)) {
            cntl->SetFailed(rc, "Fail to add timer for timeout");
            return cntl->HandleSendFailed();
        }
    } else {
        cntl->_deadline_us = -1;
    }

    cntl->IssueRPC(start_send_real_us);
    if (done == nullptr) {
        // MUST wait for response when sending synchronous RPC. It will
        // be woken up by callback when RPC finishes (succeeds or still
        // fails after retry)
        Join(correlation_id);
        if (cntl->_span) {
            cntl->SubmitSpan();
        }
        cntl->OnRPCEnd(mutil::gettimeofday_us());
    }
}

void Channel::Describe(std::ostream& os, const DescribeOptions& opt) const {
    os << "Channel[";
    if (SingleServer()) {
        os << _server_address;
    } else {
        _lb->Describe(os, opt);
    }
    os << "]";
}

int Channel::Weight() {
    return (_lb ? _lb->Weight() : 0);
}

int Channel::CheckHealth() {
    if (_lb == nullptr) {
        SocketUniquePtr ptr;
        if (Socket::Address(_server_id, &ptr) == 0 && ptr->IsAvailable()) {
            return 0;
        }
        return -1;
    } else {
        SocketUniquePtr tmp_sock;
        LoadBalancer::SelectIn sel_in = { 0, false, true, 0, nullptr };
        LoadBalancer::SelectOut sel_out(&tmp_sock);
        return _lb->SelectServer(sel_in, &sel_out);
    }
}

} // namespace melon
