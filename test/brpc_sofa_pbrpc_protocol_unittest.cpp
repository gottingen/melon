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

// brpc - A framework to host and access services throughout Baidu.

// Date: Sun Jul 13 15:04:18 CST 2014

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <google/protobuf/descriptor.h>
#include "melon/butil/time.h"
#include "melon/butil/macros.h"
#include "melon/rpc/socket.h"
#include "melon/rpc/acceptor.h"
#include "melon/rpc/server.h"
#include "melon/proto/rpc/sofa_pbrpc_meta.pb.h"
#include "melon/rpc/policy/sofa_pbrpc_protocol.h"
#include "melon/rpc/policy/most_common_message.h"
#include "melon/rpc/controller.h"
#include "echo.pb.h"

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}

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
                         const butil::EndPoint&,
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
        melon::Controller* cntl = static_cast<melon::Controller*>(cntl_base);
        melon::ClosureGuard done_guard(done);

        if (req->close_fd()) {
            cntl->CloseConnection("Close connection according to request");
            return;
        }
        EXPECT_EQ(EXP_REQUEST, req->message());
        res->set_message(EXP_RESPONSE);
    }
};
    
class SofaTest : public ::testing::Test{
protected:
    SofaTest() {
        EXPECT_EQ(0, _server.AddService(
            &_svc, melon::SERVER_DOESNT_OWN_SERVICE));
        // Hack: Regard `_server' as running 
        _server._status = melon::Server::RUNNING;
        // Sofa doesn't support authentication
        // _server._options.auth = &_auth;
        
        EXPECT_EQ(0, pipe(_pipe_fds));

        melon::SocketId id;
        melon::SocketOptions options;
        options.fd = _pipe_fds[1];
        EXPECT_EQ(0, melon::Socket::Create(options, &id));
        EXPECT_EQ(0, melon::Socket::Address(id, &_socket));
    };

    virtual ~SofaTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {};

    void VerifyMessage(melon::InputMessageBase* msg) {
        if (msg->_socket == NULL) {
            _socket->ReAddress(&msg->_socket);
        }
        msg->_arg = &_server;
        EXPECT_TRUE(melon::policy::VerifySofaRequest(msg));
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
        const melon::policy::SofaRpcMeta& meta) {
        melon::policy::MostCommonMessage* msg =
                melon::policy::MostCommonMessage::Get();
        butil::IOBufAsZeroCopyOutputStream meta_stream(&msg->meta);
        EXPECT_TRUE(meta.SerializeToZeroCopyStream(&meta_stream));

        test::EchoRequest req;
        req.set_message(EXP_REQUEST);
        butil::IOBufAsZeroCopyOutputStream req_stream(&msg->payload);
        EXPECT_TRUE(req.SerializeToZeroCopyStream(&req_stream));
        return msg;
    }

    melon::policy::MostCommonMessage* MakeResponseMessage(
        const melon::policy::SofaRpcMeta& meta) {
        melon::policy::MostCommonMessage* msg =
                melon::policy::MostCommonMessage::Get();
        butil::IOBufAsZeroCopyOutputStream meta_stream(&msg->meta);
        EXPECT_TRUE(meta.SerializeToZeroCopyStream(&meta_stream));

        test::EchoResponse res;
        res.set_message(EXP_RESPONSE);
        butil::IOBufAsZeroCopyOutputStream res_stream(&msg->payload);
        EXPECT_TRUE(res.SerializeToZeroCopyStream(&res_stream));
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
        butil::IOPortal buf;
        EXPECT_EQ((ssize_t)bytes_in_pipe,
                  buf.append_from_file_descriptor(_pipe_fds[0], 1024));
        melon::ParseResult pr = melon::policy::ParseSofaMessage(&buf, NULL, false, NULL);
        EXPECT_EQ(melon::PARSE_OK, pr.error());
        melon::policy::MostCommonMessage* msg =
            static_cast<melon::policy::MostCommonMessage*>(pr.message());

        melon::policy::SofaRpcMeta meta;
        butil::IOBufAsZeroCopyInputStream meta_stream(msg->meta);
        EXPECT_TRUE(meta.ParseFromZeroCopyStream(&meta_stream));
        EXPECT_EQ(expect_code, meta.error_code());
    }

    void TestSofaCompress(melon::CompressType type) {
        butil::IOBuf request_buf;
        butil::IOBuf total_buf;
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        cntl._response = &res;

        req.set_message(EXP_REQUEST);
        cntl.set_request_compress_type(type);
        melon::SerializeRequestDefault(&request_buf, &cntl, &req);
        ASSERT_FALSE(cntl.Failed());
        melon::policy::PackSofaRequest(
            &total_buf, NULL, cntl.call_id().value,
            test::EchoService::descriptor()->method(0),
            &cntl, request_buf, &_auth);
        ASSERT_FALSE(cntl.Failed());

        melon::ParseResult req_pr =
                melon::policy::ParseSofaMessage(&total_buf, NULL, false, NULL);
        ASSERT_EQ(melon::PARSE_OK, req_pr.error());
        melon::InputMessageBase* req_msg = req_pr.message();
        ProcessMessage(melon::policy::ProcessSofaRequest, req_msg, false);
        CheckResponseCode(false, 0);
    }

    int _pipe_fds[2];
    melon::SocketUniquePtr _socket;
    melon::Server _server;

    MyEchoService _svc;
    MyAuthenticator _auth;
};

TEST_F(SofaTest, process_request_failed_socket) {
    melon::policy::SofaRpcMeta meta;
    meta.set_type(melon::policy::SofaRpcMeta::REQUEST);
    meta.set_sequence_id(0);
    meta.set_method("EchoService.Echo");
    melon::policy::MostCommonMessage* msg = MakeRequestMessage(meta);
    _socket->SetFailed();
    ProcessMessage(melon::policy::ProcessSofaRequest, msg, false);
    ASSERT_EQ(0ll, _server._nerror_bvar.get_value());
    CheckResponseCode(true, 0);
}

TEST_F(SofaTest, process_request_logoff) {
    melon::policy::SofaRpcMeta meta;
    meta.set_type(melon::policy::SofaRpcMeta::REQUEST);
    meta.set_sequence_id(0);
    meta.set_method("EchoService.Echo");
    melon::policy::MostCommonMessage* msg = MakeRequestMessage(meta);
    _server._status = melon::Server::READY;
    ProcessMessage(melon::policy::ProcessSofaRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_bvar.get_value());
    CheckResponseCode(false, melon::ELOGOFF);
}

TEST_F(SofaTest, process_request_wrong_method) {
    melon::policy::SofaRpcMeta meta;
    meta.set_type(melon::policy::SofaRpcMeta::REQUEST);
    meta.set_sequence_id(0);
    meta.set_method("EchoService.NO_SUCH_METHOD");
    melon::policy::MostCommonMessage* msg = MakeRequestMessage(meta);
    ProcessMessage(melon::policy::ProcessSofaRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_bvar.get_value());
    CheckResponseCode(false, melon::ENOMETHOD);
}

TEST_F(SofaTest, process_response_after_eof) {
    melon::policy::SofaRpcMeta meta;
    test::EchoResponse res;
    melon::Controller cntl;
    meta.set_type(melon::policy::SofaRpcMeta::RESPONSE);
    meta.set_sequence_id(cntl.call_id().value);
    cntl._response = &res;
    melon::policy::MostCommonMessage* msg = MakeResponseMessage(meta);
    ProcessMessage(melon::policy::ProcessSofaResponse, msg, true);
    ASSERT_EQ(EXP_RESPONSE, res.message());
    ASSERT_TRUE(_socket->Failed());
}

TEST_F(SofaTest, process_response_error_code) {
    const int ERROR_CODE = 12345;
    melon::policy::SofaRpcMeta meta;
    melon::Controller cntl;
    meta.set_type(melon::policy::SofaRpcMeta::RESPONSE);
    meta.set_sequence_id(cntl.call_id().value);
    meta.set_error_code(ERROR_CODE);
    melon::policy::MostCommonMessage* msg = MakeResponseMessage(meta);
    ProcessMessage(melon::policy::ProcessSofaResponse, msg, false);
    ASSERT_EQ(ERROR_CODE, cntl.ErrorCode());
}

TEST_F(SofaTest, complete_flow) {
    butil::IOBuf request_buf;
    butil::IOBuf total_buf;
    melon::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    cntl._response = &res;

    // Send request
    req.set_message(EXP_REQUEST);
    melon::SerializeRequestDefault(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    melon::policy::PackSofaRequest(
        &total_buf, NULL, cntl.call_id().value,
        test::EchoService::descriptor()->method(0), &cntl, request_buf, &_auth);
    ASSERT_FALSE(cntl.Failed());

    // Verify and handle request
    melon::ParseResult req_pr =
            melon::policy::ParseSofaMessage(&total_buf, NULL, false, NULL);
    ASSERT_EQ(melon::PARSE_OK, req_pr.error());
    melon::InputMessageBase* req_msg = req_pr.message();
    VerifyMessage(req_msg);
    ProcessMessage(melon::policy::ProcessSofaRequest, req_msg, false);

    // Read response from pipe
    butil::IOPortal response_buf;
    response_buf.append_from_file_descriptor(_pipe_fds[0], 1024);
    melon::ParseResult res_pr =
            melon::policy::ParseSofaMessage(&response_buf, NULL, false, NULL);
    ASSERT_EQ(melon::PARSE_OK, res_pr.error());
    melon::InputMessageBase* res_msg = res_pr.message();
    ProcessMessage(melon::policy::ProcessSofaResponse, res_msg, false);

    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ(EXP_RESPONSE, res.message());
}

TEST_F(SofaTest, close_in_callback) {
    butil::IOBuf request_buf;
    butil::IOBuf total_buf;
    melon::Controller cntl;
    test::EchoRequest req;

    // Send request
    req.set_message(EXP_REQUEST);
    req.set_close_fd(true);
    melon::SerializeRequestDefault(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    melon::policy::PackSofaRequest(
        &total_buf, NULL, cntl.call_id().value,
        test::EchoService::descriptor()->method(0), &cntl, request_buf, &_auth);
    ASSERT_FALSE(cntl.Failed());

    // Handle request
    melon::ParseResult req_pr =
            melon::policy::ParseSofaMessage(&total_buf, NULL, false, NULL);
    ASSERT_EQ(melon::PARSE_OK, req_pr.error());
    melon::InputMessageBase* req_msg = req_pr.message();
    ProcessMessage(melon::policy::ProcessSofaRequest, req_msg, false);

    // Socket should be closed
    ASSERT_TRUE(_socket->Failed());
}

TEST_F(SofaTest, sofa_compress) {
    TestSofaCompress(melon::COMPRESS_TYPE_SNAPPY);
    TestSofaCompress(melon::COMPRESS_TYPE_GZIP);
    TestSofaCompress(melon::COMPRESS_TYPE_ZLIB);
}
} //namespace
