// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


// brpc - A framework to host and access services throughout Baidu.

// Date: Sun Jul 13 15:04:18 CST 2014

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <google/protobuf/descriptor.h>
#include "melon/utility/time.h"
#include "melon/utility/macros.h"
#include "melon/rpc/socket.h"
#include "melon/rpc/acceptor.h"
#include "melon/rpc/server.h"
#include "melon/rpc/policy/nova_pbrpc_protocol.h"
#include "melon/rpc/policy/most_common_message.h"
#include "melon/rpc/controller.h"
#include "echo.pb.h"

namespace {

static const std::string EXP_REQUEST = "hello";
static const std::string EXP_RESPONSE = "world";

static const std::string MOCK_CREDENTIAL = "mock credential";
static const std::string MOCK_USER = "mock user";

class MyAuthenticator : public melon::Authenticator {
public:
    MyAuthenticator() {}

    int GenerateCredential(std::string* auth_str) const {
        *auth_str = MOCK_CREDENTIAL;
        return 0;
    }

    int VerifyCredential(const std::string& auth_str,
                         const mutil::EndPoint&,
                         melon::AuthContext* ctx) const {
        EXPECT_EQ(MOCK_CREDENTIAL, auth_str);
        ctx->set_user(MOCK_USER);
        return 0;
    }
};

class MyEchoService : public ::test::EchoService {
    void Echo(::google::protobuf::RpcController* cntl_base,
              const ::test::EchoRequest* req,
              ::test::EchoResponse* res,
              ::google::protobuf::Closure* done) {
        melon::Controller* cntl =
            static_cast<melon::Controller*>(cntl_base);
        melon::ClosureGuard done_guard(done);

        if (req->close_fd()) {
            cntl->CloseConnection("Close connection according to request");
            return;
        }
        EXPECT_EQ(EXP_REQUEST, req->message());
        res->set_message(EXP_RESPONSE);
    }
};
    
class NovaTest : public ::testing::Test{
protected:
    NovaTest() {
        EXPECT_EQ(0, _server.AddService(
            &_svc, melon::SERVER_DOESNT_OWN_SERVICE));
        // Hack: Regard `_server' as running 
        _server._status = melon::Server::RUNNING;
        _server._options.nshead_service = new melon::policy::NovaServiceAdaptor;
        // Nova doesn't support authentication
        // _server._options.auth = &_auth;
        
        EXPECT_EQ(0, pipe(_pipe_fds));

        melon::SocketId id;
        melon::SocketOptions options;
        options.fd = _pipe_fds[1];
        EXPECT_EQ(0, melon::Socket::Create(options, &id));
        EXPECT_EQ(0, melon::Socket::Address(id, &_socket));
    };

    virtual ~NovaTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {};

    void VerifyMessage(melon::InputMessageBase* msg) {
        if (msg->_socket == NULL) {
            _socket->ReAddress(&msg->_socket);
        }
        msg->_arg = &_server;
        EXPECT_TRUE(melon::policy::VerifyNsheadRequest(msg));
    }

    void ProcessMessage(void (*process)(melon::InputMessageBase*),
                        melon::InputMessageBase* msg, bool set_eof) {
        if (msg->_socket == NULL) {
            _socket->ReAddress(&msg->_socket);
        }
        msg->_arg = &_server;
        _socket->PostponeEOF();
        if (set_eof) {
            _socket->SetEOF();
        }
        (*process)(msg);
    }

    melon::policy::MostCommonMessage* MakeRequestMessage(
        const melon::nshead_t& head) {
        melon::policy::MostCommonMessage* msg =
                melon::policy::MostCommonMessage::Get();
        msg->meta.append(&head, sizeof(head));

        test::EchoRequest req;
        req.set_message(EXP_REQUEST);
        mutil::IOBufAsZeroCopyOutputStream req_stream(&msg->payload);
        EXPECT_TRUE(req.SerializeToZeroCopyStream(&req_stream));
        return msg;
    }

    melon::policy::MostCommonMessage* MakeResponseMessage() {
        melon::policy::MostCommonMessage* msg =
                melon::policy::MostCommonMessage::Get();
        melon::nshead_t head;
        memset(&head, 0, sizeof(head));
        msg->meta.append(&head, sizeof(head));
        
        test::EchoResponse res;
        res.set_message(EXP_RESPONSE);
        mutil::IOBufAsZeroCopyOutputStream res_stream(&msg->payload);
        EXPECT_TRUE(res.SerializeToZeroCopyStream(&res_stream));
        return msg;
    }

    void CheckEmptyResponse() {
        int bytes_in_pipe = 0;
        ioctl(_pipe_fds[0], FIONREAD, &bytes_in_pipe);
        EXPECT_EQ(0, bytes_in_pipe);
    }

    int _pipe_fds[2];
    melon::SocketUniquePtr _socket;
    melon::Server _server;

    MyEchoService _svc;
    MyAuthenticator _auth;
};

TEST_F(NovaTest, process_request_failed_socket) {
    melon::nshead_t head;
    memset(&head, 0, sizeof(head));
    melon::policy::MostCommonMessage* msg = MakeRequestMessage(head);
    _socket->SetFailed();
    ProcessMessage(melon::policy::ProcessNsheadRequest, msg, false);
    ASSERT_EQ(0ll, _server._nerror_bvar.get_value());
    CheckEmptyResponse();
}

TEST_F(NovaTest, process_request_logoff) {
    melon::nshead_t head;
    head.reserved = 0;
    melon::policy::MostCommonMessage* msg = MakeRequestMessage(head);
    _server._status = melon::Server::READY;
    ProcessMessage(melon::policy::ProcessNsheadRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_bvar.get_value());
    ASSERT_TRUE(_socket->Failed());
    CheckEmptyResponse();
}

TEST_F(NovaTest, process_request_wrong_method) {
    melon::nshead_t head;
    head.reserved = 10;
    melon::policy::MostCommonMessage* msg = MakeRequestMessage(head);
    ProcessMessage(melon::policy::ProcessNsheadRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_bvar.get_value());
    ASSERT_TRUE(_socket->Failed());
    CheckEmptyResponse();
}

TEST_F(NovaTest, process_response_after_eof) {
    test::EchoResponse res;
    melon::Controller cntl;
    cntl._response = &res;
    melon::policy::MostCommonMessage* msg = MakeResponseMessage();
    _socket->set_correlation_id(cntl.call_id().value);
    ProcessMessage(melon::policy::ProcessNovaResponse, msg, true);
    ASSERT_EQ(EXP_RESPONSE, res.message());
    ASSERT_TRUE(_socket->Failed());
}

TEST_F(NovaTest, complete_flow) {
    mutil::IOBuf request_buf;
    mutil::IOBuf total_buf;
    melon::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    cntl._response = &res;
    cntl._connection_type = melon::CONNECTION_TYPE_SHORT;
    ASSERT_EQ(0, melon::Socket::Address(_socket->id(), &cntl._current_call.sending_sock));

    // Send request
    req.set_message(EXP_REQUEST);
    melon::SerializeRequestDefault(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    melon::policy::PackNovaRequest(
        &total_buf, NULL, cntl.call_id().value,
        test::EchoService::descriptor()->method(0), &cntl, request_buf, &_auth);
    ASSERT_FALSE(cntl.Failed());

    // Verify and handle request
    melon::ParseResult req_pr =
            melon::policy::ParseNsheadMessage(&total_buf, NULL, false, NULL);
    ASSERT_EQ(melon::PARSE_OK, req_pr.error());
    melon::InputMessageBase* req_msg = req_pr.message();
    VerifyMessage(req_msg);
    ProcessMessage(melon::policy::ProcessNsheadRequest, req_msg, false);

    // Read response from pipe
    mutil::IOPortal response_buf;
    response_buf.append_from_file_descriptor(_pipe_fds[0], 1024);
    melon::ParseResult res_pr =
            melon::policy::ParseNsheadMessage(&response_buf, NULL, false, NULL);
    ASSERT_EQ(melon::PARSE_OK, res_pr.error());
    melon::InputMessageBase* res_msg = res_pr.message();
    ProcessMessage(melon::policy::ProcessNovaResponse, res_msg, false);

    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(EXP_RESPONSE, res.message());
}

TEST_F(NovaTest, close_in_callback) {
    mutil::IOBuf request_buf;
    mutil::IOBuf total_buf;
    melon::Controller cntl;
    test::EchoRequest req;
    cntl._connection_type = melon::CONNECTION_TYPE_SHORT;
    ASSERT_EQ(0, melon::Socket::Address(_socket->id(), &cntl._current_call.sending_sock));

    // Send request
    req.set_message(EXP_REQUEST);
    req.set_close_fd(true);
    melon::SerializeRequestDefault(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    melon::policy::PackNovaRequest(
        &total_buf, NULL, cntl.call_id().value,
        test::EchoService::descriptor()->method(0), &cntl, request_buf, &_auth);
    ASSERT_FALSE(cntl.Failed());

    // Handle request
    melon::ParseResult req_pr =
            melon::policy::ParseNsheadMessage(&total_buf, NULL, false, NULL);
    ASSERT_EQ(melon::PARSE_OK, req_pr.error());
    melon::InputMessageBase* req_msg = req_pr.message();
    ProcessMessage(melon::policy::ProcessNsheadRequest, req_msg, false);

    // Socket should be closed
    ASSERT_TRUE(_socket->Failed());
}
} //namespace
