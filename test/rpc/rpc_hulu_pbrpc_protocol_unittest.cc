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




// Date: Sun Jul 13 15:04:18 CST 2014

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <google/protobuf/descriptor.h>
#include "melon/utility/time.h"
#include "melon/utility/macros.h"
#include "melon/utility/gperftools_profiler.h"
#include "melon/rpc/socket.h"
#include "melon/rpc/acceptor.h"
#include "melon/rpc/server.h"
#include "melon/proto/rpc/hulu_pbrpc_meta.pb.h"
#include "melon/rpc/policy/hulu_pbrpc_protocol.h"
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
        if (cntl->auth_context()) {
            EXPECT_EQ(MOCK_USER, cntl->auth_context()->user());
        }
        EXPECT_EQ(EXP_REQUEST, req->message());
        if (!cntl->request_attachment().empty()) {
            EXPECT_EQ(EXP_REQUEST, cntl->request_attachment().to_string());
            cntl->response_attachment().append(EXP_RESPONSE);
        }
        res->set_message(EXP_RESPONSE);
    }
};
    
class HuluTest : public ::testing::Test{
protected:
    HuluTest() {
        EXPECT_EQ(0, _server.AddService(
            &_svc, melon::SERVER_DOESNT_OWN_SERVICE));
        // Hack: Regard `_server' as running 
        _server._status = melon::Server::RUNNING;
        _server._options.auth = &_auth;
        
        EXPECT_EQ(0, pipe(_pipe_fds));

        melon::SocketId id;
        melon::SocketOptions options;
        options.fd = _pipe_fds[1];
        EXPECT_EQ(0, melon::Socket::Create(options, &id));
        EXPECT_EQ(0, melon::Socket::Address(id, &_socket));
    };

    virtual ~HuluTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {};

    void VerifyMessage(melon::InputMessageBase* msg) {
        if (msg->_socket == NULL) {
            _socket->ReAddress(&msg->_socket);
        }
        msg->_arg = &_server;
        EXPECT_TRUE(melon::policy::VerifyHuluRequest(msg));
    }

    void ProcessMessage(void (*process)(melon::InputMessageBase*),
                        melon::InputMessageBase* msg, bool set_eof) {
        if (msg->_socket == NULL) {
            _socket.get()->ReAddress(&msg->_socket);
        }
        msg->_arg = &_server;
        _socket->PostponeEOF();
        if (set_eof) {
            _socket->SetEOF();
        }
        (*process)(msg);
    }

    melon::policy::MostCommonMessage* MakeRequestMessage(
        const melon::policy::HuluRpcRequestMeta& meta) {
        melon::policy::MostCommonMessage* msg =
                melon::policy::MostCommonMessage::Get();
        mutil::IOBufAsZeroCopyOutputStream meta_stream(&msg->meta);
        EXPECT_TRUE(meta.SerializeToZeroCopyStream(&meta_stream));

        test::EchoRequest req;
        req.set_message(EXP_REQUEST);
        mutil::IOBufAsZeroCopyOutputStream req_stream(&msg->payload);
        EXPECT_TRUE(req.SerializeToZeroCopyStream(&req_stream));
        return msg;
    }

    melon::policy::MostCommonMessage* MakeResponseMessage(
        const melon::policy::HuluRpcResponseMeta& meta) {
        melon::policy::MostCommonMessage* msg =
                melon::policy::MostCommonMessage::Get();
        mutil::IOBufAsZeroCopyOutputStream meta_stream(&msg->meta);
        EXPECT_TRUE(meta.SerializeToZeroCopyStream(&meta_stream));

        test::EchoResponse res;
        res.set_message(EXP_RESPONSE);
        mutil::IOBufAsZeroCopyOutputStream res_stream(&msg->payload);
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
        mutil::IOPortal buf;
        EXPECT_EQ((ssize_t)bytes_in_pipe,
                  buf.append_from_file_descriptor(_pipe_fds[0], 1024));
        melon::ParseResult pr = melon::policy::ParseHuluMessage(&buf, NULL, false, NULL);
        EXPECT_EQ(melon::PARSE_OK, pr.error());
        melon::policy::MostCommonMessage* msg =
            static_cast<melon::policy::MostCommonMessage*>(pr.message());

        melon::policy::HuluRpcResponseMeta meta;
        mutil::IOBufAsZeroCopyInputStream meta_stream(msg->meta);
        EXPECT_TRUE(meta.ParseFromZeroCopyStream(&meta_stream));
        EXPECT_EQ(expect_code, meta.error_code());
    }

    void TestHuluCompress(melon::CompressType type) {
        mutil::IOBuf request_buf;
        mutil::IOBuf total_buf;
        melon::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        cntl._response = &res;

        req.set_message(EXP_REQUEST);
        cntl.set_request_compress_type(type);
        melon::SerializeRequestDefault(&request_buf, &cntl, &req);
        ASSERT_FALSE(cntl.Failed());
        melon::policy::PackHuluRequest(
            &total_buf, NULL, cntl.call_id().value,
            test::EchoService::descriptor()->method(0),
            &cntl, request_buf, &_auth);
        ASSERT_FALSE(cntl.Failed());

        melon::ParseResult req_pr =
                melon::policy::ParseHuluMessage(&total_buf, NULL, false, NULL);
        ASSERT_EQ(melon::PARSE_OK, req_pr.error());
        melon::InputMessageBase* req_msg = req_pr.message();
        ProcessMessage(melon::policy::ProcessHuluRequest, req_msg, false);
        CheckResponseCode(false, 0);
    }

    int _pipe_fds[2];
    melon::SocketUniquePtr _socket;
    melon::Server _server;

    MyEchoService _svc;
    MyAuthenticator _auth;
};

TEST_F(HuluTest, process_request_failed_socket) {
    melon::policy::HuluRpcRequestMeta meta;
    meta.set_service_name("EchoService");
    meta.set_method_index(0);
    melon::policy::MostCommonMessage* msg = MakeRequestMessage(meta);
    _socket->SetFailed();
    ProcessMessage(melon::policy::ProcessHuluRequest, msg, false);
    ASSERT_EQ(0ll, _server._nerror_var.get_value());
    CheckResponseCode(true, 0);
}

TEST_F(HuluTest, process_request_logoff) {
    melon::policy::HuluRpcRequestMeta meta;
    meta.set_service_name("EchoService");
    meta.set_method_index(0);
    melon::policy::MostCommonMessage* msg = MakeRequestMessage(meta);
    _server._status = melon::Server::READY;
    ProcessMessage(melon::policy::ProcessHuluRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_var.get_value());
    CheckResponseCode(false, melon::ELOGOFF);
}

TEST_F(HuluTest, process_request_wrong_method) {
    melon::policy::HuluRpcRequestMeta meta;
    meta.set_service_name("EchoService");
    meta.set_method_index(10);
    melon::policy::MostCommonMessage* msg = MakeRequestMessage(meta);
    ProcessMessage(melon::policy::ProcessHuluRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_var.get_value());
    CheckResponseCode(false, melon::ENOMETHOD);
}

TEST_F(HuluTest, process_response_after_eof) {
    melon::policy::HuluRpcResponseMeta meta;
    test::EchoResponse res;
    melon::Controller cntl;
    meta.set_correlation_id(cntl.call_id().value);
    cntl._response = &res;
    melon::policy::MostCommonMessage* msg = MakeResponseMessage(meta);
    ProcessMessage(melon::policy::ProcessHuluResponse, msg, true);
    ASSERT_EQ(EXP_RESPONSE, res.message());
    ASSERT_TRUE(_socket->Failed());
}

TEST_F(HuluTest, process_response_error_code) {
    const int ERROR_CODE = 12345;
    melon::policy::HuluRpcResponseMeta meta;
    melon::Controller cntl;
    meta.set_correlation_id(cntl.call_id().value);
    meta.set_error_code(ERROR_CODE);
    melon::policy::MostCommonMessage* msg = MakeResponseMessage(meta);
    ProcessMessage(melon::policy::ProcessHuluResponse, msg, false);
    ASSERT_EQ(ERROR_CODE, cntl.ErrorCode());
}

TEST_F(HuluTest, complete_flow) {
    mutil::IOBuf request_buf;
    mutil::IOBuf total_buf;
    melon::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    cntl._response = &res;

    // Send request
    req.set_message(EXP_REQUEST);
    melon::SerializeRequestDefault(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    cntl.request_attachment().append(EXP_REQUEST);
    melon::policy::PackHuluRequest(
        &total_buf, NULL, cntl.call_id().value,
        test::EchoService::descriptor()->method(0), &cntl, request_buf, &_auth);
    ASSERT_FALSE(cntl.Failed());

    // Verify and handle request
    melon::ParseResult req_pr =
            melon::policy::ParseHuluMessage(&total_buf, NULL, false, NULL);
    ASSERT_EQ(melon::PARSE_OK, req_pr.error());
    melon::InputMessageBase* req_msg = req_pr.message();
    VerifyMessage(req_msg);
    ProcessMessage(melon::policy::ProcessHuluRequest, req_msg, false);

    // Read response from pipe
    mutil::IOPortal response_buf;
    response_buf.append_from_file_descriptor(_pipe_fds[0], 1024);
    melon::ParseResult res_pr =
            melon::policy::ParseHuluMessage(&response_buf, NULL, false, NULL);
    ASSERT_EQ(melon::PARSE_OK, res_pr.error());
    melon::InputMessageBase* res_msg = res_pr.message();
    ProcessMessage(melon::policy::ProcessHuluResponse, res_msg, false);

    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ(EXP_RESPONSE, res.message());
}

TEST_F(HuluTest, close_in_callback) {
    mutil::IOBuf request_buf;
    mutil::IOBuf total_buf;
    melon::Controller cntl;
    test::EchoRequest req;

    // Send request
    req.set_message(EXP_REQUEST);
    req.set_close_fd(true);
    melon::SerializeRequestDefault(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    melon::policy::PackHuluRequest(
        &total_buf, NULL, cntl.call_id().value,
        test::EchoService::descriptor()->method(0), &cntl, request_buf, &_auth);
    ASSERT_FALSE(cntl.Failed());

    // Handle request
    melon::ParseResult req_pr =
            melon::policy::ParseHuluMessage(&total_buf, NULL, false, NULL);
    ASSERT_EQ(melon::PARSE_OK, req_pr.error());
    melon::InputMessageBase* req_msg = req_pr.message();
    ProcessMessage(melon::policy::ProcessHuluRequest, req_msg, false);

    // Socket should be closed
    ASSERT_TRUE(_socket->Failed());
}

TEST_F(HuluTest, hulu_compress) {
    TestHuluCompress(melon::COMPRESS_TYPE_SNAPPY);
    TestHuluCompress(melon::COMPRESS_TYPE_GZIP);
    TestHuluCompress(melon::COMPRESS_TYPE_ZLIB);
}
} //namespace
