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

// This is an rpc-internal file.

#include <melon/rpc/socket.h>
#include <melon/rpc/controller.h>
#include <melon/rpc/stream.h>

namespace google {
namespace protobuf {
class Message;
}
}


namespace melon {

class AuthContext;

// A wrapper to access some private methods/fields of `Controller'
// This is supposed to be used by internal RPC protocols ONLY
class ControllerPrivateAccessor {
public:
    explicit ControllerPrivateAccessor(Controller* cntl) {
        _cntl = cntl;
    }

    void OnResponse(CallId id, int saved_error) {
        const Controller::CompletionInfo info = { id, true };
        _cntl->OnVersionedRPCReturned(info, false, saved_error);
    }

    ControllerPrivateAccessor &set_peer_id(SocketId peer_id) {
        _cntl->_current_call.peer_id = peer_id;
        return *this;
    }

    Socket* get_sending_socket() {
        return _cntl->_current_call.sending_sock.get();
    }

    int64_t real_timeout_ms() {
        return _cntl->_real_timeout_ms;
    }

    void move_in_server_receiving_sock(SocketUniquePtr& ptr) {
        CHECK(_cntl->_current_call.sending_sock == NULL);
        _cntl->_current_call.sending_sock.reset(ptr.release());
    }

    StreamUserData* get_stream_user_data() {
        return _cntl->_current_call.stream_user_data;
    }

    ControllerPrivateAccessor &set_security_mode(bool security_mode) {
        _cntl->set_flag(Controller::FLAGS_SECURITY_MODE, security_mode);
        return *this;
    }

    ControllerPrivateAccessor &set_remote_side(const mutil::EndPoint& pt) {
        _cntl->_remote_side = pt;
        return *this;
    }

    ControllerPrivateAccessor &set_local_side(const mutil::EndPoint& pt) {
        _cntl->_local_side = pt;
        return *this;
    }
 
    ControllerPrivateAccessor &set_auth_context(const AuthContext* ctx) {
        _cntl->set_auth_context(ctx);
        return *this;
    }

    ControllerPrivateAccessor &set_span(Span* span) {
        _cntl->_span = span;
        return *this;
    }
    
    ControllerPrivateAccessor &set_request_protocol(ProtocolType protocol) {
        _cntl->_request_protocol = protocol;
        return *this;
    }
    
    Span* span() const { return _cntl->_span; }

    uint32_t pipelined_count() const { return _cntl->_pipelined_count; }
    void set_pipelined_count(uint32_t count) {  _cntl->_pipelined_count = count; }

    ControllerPrivateAccessor& set_server(const Server* server) {
        _cntl->_server = server;
        return *this;
    }

    // Pass the owership of |settings| to _cntl, while is going to be
    // destroyed in Controller::Reset()
    void set_remote_stream_settings(StreamSettings *settings) {
        _cntl->_remote_stream_settings = settings;
    }
    StreamSettings* remote_stream_settings() {
        return _cntl->_remote_stream_settings;
    }

    StreamId request_stream() { return _cntl->_request_stream; }
    StreamId response_stream() { return _cntl->_response_stream; }

    void set_method(const google::protobuf::MethodDescriptor* method) 
    { _cntl->_method = method; }

    void set_readable_progressive_attachment(ReadableProgressiveAttachment* s)
    { _cntl->_rpa.reset(s); }

    void set_auth_flags(uint32_t auth_flags) {
        _cntl->_auth_flags = auth_flags;
    }

    void clear_auth_flags() { _cntl->_auth_flags = 0; }

    std::string& protocol_param() { return _cntl->protocol_param(); }
    const std::string& protocol_param() const { return _cntl->protocol_param(); }

    // Note: This function can only be called in server side. The deadline of client
    // side is properly set in the RPC sending path.
    void set_deadline_us(int64_t deadline_us) { _cntl->_deadline_us = deadline_us; }

    ControllerPrivateAccessor& set_begin_time_us(int64_t begin_time_us) {
        _cntl->_begin_time_us = begin_time_us;
        _cntl->_end_time_us = UNSET_MAGIC_NUM;
        return *this;
    }

    ControllerPrivateAccessor& set_health_check_call() {
        _cntl->add_flag(Controller::FLAGS_HEALTH_CHECK_CALL);
        return *this;
    }

private:
    Controller* _cntl;
};

// Inherit this class to intercept Controller::IssueRPC. This is an internal
// utility only useable by melon developers.
class RPCSender {
public:
    virtual ~RPCSender() {}
    virtual int IssueRPC(int64_t start_realtime_us) = 0;
};

} // namespace melon

