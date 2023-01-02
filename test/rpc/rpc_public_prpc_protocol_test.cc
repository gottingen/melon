// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// Date: Sun Jul 13 15:04:18 CST 2014

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "testing/gtest_wrap.h"
#include <gflags/gflags.h>
#include <google/protobuf/descriptor.h>
#include "melon/times/time.h"
#include "melon/rpc/socket.h"
#include "melon/rpc/acceptor.h"
#include "melon/rpc/server.h"
#include "melon/rpc/policy/public_pbrpc_meta.pb.h"
#include "melon/rpc/policy/public_pbrpc_protocol.h"
#include "melon/rpc/policy/most_common_message.h"
#include "melon/rpc/controller.h"
#include "echo.pb.h"

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}

namespace {

static const std::string EXP_REQUEST = "hello";
static const std::string EXP_RESPONSE = "world";

static const std::string MOCK_CREDENTIAL = "mock credential";
static const std::string MOCK_USER = "mock user";

class MyAuthenticator : public melon::rpc::Authenticator {
public:
    MyAuthenticator() {}

    int GenerateCredential(std::string* auth_str) const {
        *auth_str = MOCK_CREDENTIAL;
        return 0;
    }

    int VerifyCredential(const std::string& auth_str,
                         const melon::end_point&,
                         melon::rpc::AuthContext* ctx) const {
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
        melon::rpc::Controller* cntl = static_cast<melon::rpc::Controller*>(cntl_base);
        melon::rpc::ClosureGuard done_guard(done);

        if (req->close_fd()) {
            cntl->CloseConnection("Close connection according to request");
            return;
        }
        EXPECT_EQ(EXP_REQUEST, req->message());
        res->set_message(EXP_RESPONSE);
    }
};
    
class PublicPbrpcTest : public ::testing::Test{
protected:
    PublicPbrpcTest() {
        EXPECT_EQ(0, _server.AddService(
            &_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        // Hack: Regard `_server' as running 
        _server._status = melon::rpc::Server::RUNNING;
        _server._options.nshead_service =
            new melon::rpc::policy::PublicPbrpcServiceAdaptor;
        // public_pbrpc doesn't support authentication
        // _server._options.auth = &_auth;
        
        EXPECT_EQ(0, pipe(_pipe_fds));

        melon::rpc::SocketId id;
        melon::rpc::SocketOptions options;
        options.fd = _pipe_fds[1];
        EXPECT_EQ(0, melon::rpc::Socket::Create(options, &id));
        EXPECT_EQ(0, melon::rpc::Socket::Address(id, &_socket));
    };

    virtual ~PublicPbrpcTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {};

    void VerifyMessage(melon::rpc::InputMessageBase* msg) {
        if (msg->_socket == nullptr) {
            _socket->ReAddress(&msg->_socket);
        }
        msg->_arg = &_server;
        EXPECT_TRUE(melon::rpc::policy::VerifyNsheadRequest(msg));
    }

    void ProcessMessage(void (*process)(melon::rpc::InputMessageBase*),
                        melon::rpc::InputMessageBase* msg, bool set_eof) {
        if (msg->_socket == nullptr) {
            _socket->ReAddress(&msg->_socket);
        }
        msg->_arg = &_server;
        _socket->PostponeEOF();
        if (set_eof) {
            _socket->SetEOF();
        }
        (*process)(msg);
    }
    
    melon::rpc::policy::MostCommonMessage* MakeRequestMessage(
        melon::rpc::policy::PublicPbrpcRequest* meta) {
        melon::rpc::policy::MostCommonMessage* msg =
                melon::rpc::policy::MostCommonMessage::Get();
        melon::rpc::nshead_t head;
        msg->meta.append(&head, sizeof(head));

        if (meta->requestbody_size() > 0) {
            test::EchoRequest req;
            req.set_message(EXP_REQUEST);
            EXPECT_TRUE(req.SerializeToString(
                meta->mutable_requestbody(0)->mutable_serialized_request()));
        }
        melon::cord_buf_as_zero_copy_output_stream meta_stream(&msg->payload);
        EXPECT_TRUE(meta->SerializeToZeroCopyStream(&meta_stream));
        return msg;
    }

    melon::rpc::policy::MostCommonMessage* MakeResponseMessage(
        melon::rpc::policy::PublicPbrpcResponse* meta) {
        melon::rpc::policy::MostCommonMessage* msg =
                melon::rpc::policy::MostCommonMessage::Get();
        melon::rpc::nshead_t head;
        msg->meta.append(&head, sizeof(head));

        if (meta->responsebody_size() > 0) {
            test::EchoResponse res;
            res.set_message(EXP_RESPONSE);
            EXPECT_TRUE(res.SerializeToString(
                meta->mutable_responsebody(0)->mutable_serialized_response()));
        }
        melon::cord_buf_as_zero_copy_output_stream meta_stream(&msg->payload);
        EXPECT_TRUE(meta->SerializeToZeroCopyStream(&meta_stream));
        return msg;
    }

    void CheckResponseCode(bool expect_empty, int expect_code) {
        int bytes_in_pipe = 0;
        ioctl(_pipe_fds[0], FIONREAD, &bytes_in_pipe);
        if (expect_empty) {
            EXPECT_EQ(0, bytes_in_pipe);
            return;
        }

        EXPECT_GT(bytes_in_pipe, 0);
        melon::IOPortal buf;
        EXPECT_EQ((ssize_t)bytes_in_pipe,
                  buf.append_from_file_descriptor(_pipe_fds[0], 1024));
        melon::rpc::ParseResult pr = melon::rpc::policy::ParseNsheadMessage(&buf, nullptr, false, nullptr);
        EXPECT_EQ(melon::rpc::PARSE_OK, pr.error());
        melon::rpc::policy::MostCommonMessage* msg =
            static_cast<melon::rpc::policy::MostCommonMessage*>(pr.message());

        melon::rpc::policy::PublicPbrpcResponse meta;
        melon::cord_buf_as_zero_copy_input_stream meta_stream(msg->payload);
        EXPECT_TRUE(meta.ParseFromZeroCopyStream(&meta_stream));
        EXPECT_EQ(expect_code, meta.responsehead().code());
    }

    int _pipe_fds[2];
    melon::rpc::SocketUniquePtr _socket;
    melon::rpc::Server _server;

    MyEchoService _svc;
    MyAuthenticator _auth;
};

TEST_F(PublicPbrpcTest, process_request_failed_socket) {
    melon::rpc::policy::PublicPbrpcRequest meta;
    melon::rpc::policy::RequestBody* body = meta.add_requestbody();
    body->set_service("EchoService");
    body->set_method_id(0);
    body->set_id(0);
    melon::rpc::policy::MostCommonMessage* msg = MakeRequestMessage(&meta);
    _socket->SetFailed();
    ProcessMessage(melon::rpc::policy::ProcessNsheadRequest, msg, false);
    ASSERT_EQ(0ll, _server._nerror_var.get_value());
    CheckResponseCode(true, 0);
}

TEST_F(PublicPbrpcTest, process_request_logoff) {
    melon::rpc::policy::PublicPbrpcRequest meta;
    melon::rpc::policy::RequestBody* body = meta.add_requestbody();
    body->set_service("EchoService");
    body->set_method_id(0);
    body->set_id(0);
    melon::rpc::policy::MostCommonMessage* msg = MakeRequestMessage(&meta);
    _server._status = melon::rpc::Server::READY;
    ProcessMessage(melon::rpc::policy::ProcessNsheadRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_var.get_value());
    CheckResponseCode(false, melon::rpc::ELOGOFF);
}

TEST_F(PublicPbrpcTest, process_request_wrong_method) {
    melon::rpc::policy::PublicPbrpcRequest meta;
    melon::rpc::policy::RequestBody* body = meta.add_requestbody();
    body->set_service("EchoService");
    body->set_method_id(10);
    body->set_id(0);
    melon::rpc::policy::MostCommonMessage* msg = MakeRequestMessage(&meta);
    ProcessMessage(melon::rpc::policy::ProcessNsheadRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_var.get_value());
    ASSERT_FALSE(_socket->Failed());
}

TEST_F(PublicPbrpcTest, process_response_after_eof) {
    melon::rpc::policy::PublicPbrpcResponse meta;
    test::EchoResponse res;
    melon::rpc::Controller cntl;
    melon::rpc::policy::ResponseBody* body = meta.add_responsebody();
    body->set_id(cntl.call_id().value);
    meta.mutable_responsehead()->set_code(0);
    cntl._response = &res;
    melon::rpc::policy::MostCommonMessage* msg = MakeResponseMessage(&meta);
    ProcessMessage(melon::rpc::policy::ProcessPublicPbrpcResponse, msg, true);
    ASSERT_EQ(EXP_RESPONSE, res.message());
    ASSERT_TRUE(_socket->Failed());
}

TEST_F(PublicPbrpcTest, process_response_error_code) {
    const int ERROR_CODE = 12345;
    melon::rpc::policy::PublicPbrpcResponse meta;
    melon::rpc::Controller cntl;
    melon::rpc::policy::ResponseBody* body = meta.add_responsebody();
    body->set_id(cntl.call_id().value);
    meta.mutable_responsehead()->set_code(ERROR_CODE);
    melon::rpc::policy::MostCommonMessage* msg = MakeResponseMessage(&meta);
    ProcessMessage(melon::rpc::policy::ProcessPublicPbrpcResponse, msg, false);
    ASSERT_EQ(ERROR_CODE, cntl.ErrorCode());
}

TEST_F(PublicPbrpcTest, complete_flow) {
    melon::cord_buf request_buf;
    melon::cord_buf total_buf;
    melon::rpc::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    cntl._response = &res;

    // Send request
    req.set_message(EXP_REQUEST);
    cntl.set_request_compress_type(melon::rpc::COMPRESS_TYPE_SNAPPY);
    melon::rpc::policy::SerializePublicPbrpcRequest(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    melon::rpc::policy::PackPublicPbrpcRequest(
        &total_buf, nullptr, cntl.call_id().value,
        test::EchoService::descriptor()->method(0), &cntl, request_buf, &_auth);
    ASSERT_FALSE(cntl.Failed());

    // Verify and handle request
    melon::rpc::ParseResult req_pr =
            melon::rpc::policy::ParseNsheadMessage(&total_buf, nullptr, false, nullptr);
    ASSERT_EQ(melon::rpc::PARSE_OK, req_pr.error());
    melon::rpc::InputMessageBase* req_msg = req_pr.message();
    VerifyMessage(req_msg);
    ProcessMessage(melon::rpc::policy::ProcessNsheadRequest, req_msg, false);

    // Read response from pipe
    melon::IOPortal response_buf;
    response_buf.append_from_file_descriptor(_pipe_fds[0], 1024);
    melon::rpc::ParseResult res_pr =
            melon::rpc::policy::ParseNsheadMessage(&response_buf, nullptr, false, nullptr);
    ASSERT_EQ(melon::rpc::PARSE_OK, res_pr.error());
    melon::rpc::InputMessageBase* res_msg = res_pr.message();
    ProcessMessage(melon::rpc::policy::ProcessPublicPbrpcResponse, res_msg, false);

    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ(EXP_RESPONSE, res.message());
}

TEST_F(PublicPbrpcTest, close_in_callback) {
    melon::cord_buf request_buf;
    melon::cord_buf total_buf;
    melon::rpc::Controller cntl;
    test::EchoRequest req;

    // Send request
    req.set_message(EXP_REQUEST);
    req.set_close_fd(true);
    melon::rpc::policy::SerializePublicPbrpcRequest(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    melon::rpc::policy::PackPublicPbrpcRequest(
        &total_buf, nullptr, cntl.call_id().value,
        test::EchoService::descriptor()->method(0), &cntl, request_buf, &_auth);
    ASSERT_FALSE(cntl.Failed());

    // Handle request
    melon::rpc::ParseResult req_pr =
            melon::rpc::policy::ParseNsheadMessage(&total_buf, nullptr, false, nullptr);
    ASSERT_EQ(melon::rpc::PARSE_OK, req_pr.error());
    melon::rpc::InputMessageBase* req_msg = req_pr.message();
    ProcessMessage(melon::rpc::policy::ProcessNsheadRequest, req_msg, false);

    // Socket should be closed
    ASSERT_TRUE(_socket->Failed());
}
} //namespace
