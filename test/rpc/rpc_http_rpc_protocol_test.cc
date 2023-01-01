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
#include <google/protobuf/text_format.h>
#include "melon/times/time.h"
#include "melon/files/sequential_read_file.h"
#include "melon/base/fd_guard.h"
#include "melon/rpc/socket.h"
#include "melon/rpc/acceptor.h"
#include "melon/rpc/server.h"
#include "melon/rpc/channel.h"
#include "melon/rpc/policy/most_common_message.h"
#include "melon/rpc/controller.h"
#include "echo.pb.h"
#include "melon/rpc/policy/http_rpc_protocol.h"
#include "melon/rpc/policy/http2_rpc_protocol.h"
#include "melon/json2pb/pb_to_json.h"
#include "melon/json2pb/json_to_pb.h"
#include "melon/rpc/details/method_status.h"
#include "melon/strings/starts_with.h"
#include "melon/strings/ends_with.h"
#include "melon/fiber/this_fiber.h"
#include "melon/rpc/rpc_dump.h"
#include "melon/metrics/collector.h"

namespace melon::rpc {
    DECLARE_bool(rpc_dump);
    DECLARE_string(rpc_dump_dir);
    DECLARE_int32(rpc_dump_max_requests_in_one_file);
    extern melon::CollectorSpeedLimit g_rpc_dump_sl;
}

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    if (google::SetCommandLineOption("socket_max_unwritten_bytes", "2000000").empty()) {
        std::cerr << "Fail to set -socket_max_unwritten_bytes" << std::endl;
        return -1;
    }
    if (google::SetCommandLineOption("melon_crash_on_fatal_log", "true").empty()) {
        std::cerr << "Fail to set -melon_crash_on_fatal_log" << std::endl;
        return -1;
    }
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

        int GenerateCredential(std::string *auth_str) const {
            *auth_str = MOCK_CREDENTIAL;
            return 0;
        }

        int VerifyCredential(const std::string &auth_str,
                             const melon::base::end_point &,
                             melon::rpc::AuthContext *ctx) const {
            EXPECT_EQ(MOCK_CREDENTIAL, auth_str);
            ctx->set_user(MOCK_USER);
            return 0;
        }
    };

    class MyEchoService : public ::test::EchoService {
    public:
        void Echo(::google::protobuf::RpcController *cntl_base,
                  const ::test::EchoRequest *req,
                  ::test::EchoResponse *res,
                  ::google::protobuf::Closure *done) {
            melon::rpc::ClosureGuard done_guard(done);
            melon::rpc::Controller *cntl =
                    static_cast<melon::rpc::Controller *>(cntl_base);
            const std::string *sleep_ms_str =
                    cntl->http_request().uri().GetQuery("sleep_ms");
            if (sleep_ms_str) {
                melon::fiber_sleep_for(strtol(sleep_ms_str->data(), nullptr, 10) * 1000);
            }
            res->set_message(EXP_RESPONSE);
        }
    };

    class HttpTest : public ::testing::Test {
    protected:

        HttpTest() {
            EXPECT_EQ(0, _server.AddBuiltinServices());
            EXPECT_EQ(0, _server.AddService(
                    &_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
            // Hack: Regard `_server' as running
            _server._status = melon::rpc::Server::RUNNING;
            _server._options.auth = &_auth;

            EXPECT_EQ(0, pipe(_pipe_fds));

            melon::rpc::SocketId id;
            melon::rpc::SocketOptions options;
            options.fd = _pipe_fds[1];
            EXPECT_EQ(0, melon::rpc::Socket::Create(options, &id));
            EXPECT_EQ(0, melon::rpc::Socket::Address(id, &_socket));

            melon::rpc::SocketOptions h2_client_options;
            h2_client_options.user = melon::rpc::get_client_side_messenger();
            h2_client_options.fd = _pipe_fds[1];
            EXPECT_EQ(0, melon::rpc::Socket::Create(h2_client_options, &id));
            EXPECT_EQ(0, melon::rpc::Socket::Address(id, &_h2_client_sock));
        };

        virtual ~HttpTest() {};

        virtual void SetUp() {};

        virtual void TearDown() {};

        void VerifyMessage(melon::rpc::InputMessageBase *msg, bool expect) {
            if (msg->_socket == nullptr) {
                _socket->ReAddress(&msg->_socket);
            }
            msg->_arg = &_server;
            EXPECT_EQ(expect, melon::rpc::policy::VerifyHttpRequest(msg));
        }

        void ProcessMessage(void (*process)(melon::rpc::InputMessageBase *),
                            melon::rpc::InputMessageBase *msg, bool set_eof) {
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

        melon::rpc::policy::HttpContext *MakePostRequestMessage(const std::string &path) {
            melon::rpc::policy::HttpContext *msg = new melon::rpc::policy::HttpContext(false);
            msg->header().uri().set_path(path);
            msg->header().set_content_type("application/json");
            msg->header().set_method(melon::rpc::HTTP_METHOD_POST);

            test::EchoRequest req;
            req.set_message(EXP_REQUEST);
            melon::cord_buf_as_zero_copy_output_stream req_stream(&msg->body());
            EXPECT_TRUE(json2pb::ProtoMessageToJson(req, &req_stream, nullptr));
            return msg;
        }

        melon::rpc::policy::HttpContext* MakePostProtoTextRequestMessage(
                const std::string& path) {
            melon::rpc::policy::HttpContext* msg = new melon::rpc::policy::HttpContext(false);
            msg->header().uri().set_path(path);
            msg->header().set_content_type("application/proto-text");
            msg->header().set_method(melon::rpc::HTTP_METHOD_POST);

            test::EchoRequest req;
            req.set_message(EXP_REQUEST);
            melon::cord_buf_as_zero_copy_output_stream req_stream(&msg->body());
            EXPECT_TRUE(google::protobuf::TextFormat::Print(req, &req_stream));
            return msg;
        }

        melon::rpc::policy::HttpContext *MakeGetRequestMessage(const std::string &path) {
            melon::rpc::policy::HttpContext *msg = new melon::rpc::policy::HttpContext(false);
            msg->header().uri().set_path(path);
            msg->header().set_method(melon::rpc::HTTP_METHOD_GET);
            return msg;
        }


        melon::rpc::policy::HttpContext *MakeResponseMessage(int code) {
            melon::rpc::policy::HttpContext *msg = new melon::rpc::policy::HttpContext(false);
            msg->header().set_status_code(code);
            msg->header().set_content_type("application/json");

            test::EchoResponse res;
            res.set_message(EXP_RESPONSE);
            melon::cord_buf_as_zero_copy_output_stream res_stream(&msg->body());
            EXPECT_TRUE(json2pb::ProtoMessageToJson(res, &res_stream, nullptr));
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
            EXPECT_EQ((ssize_t) bytes_in_pipe,
                      buf.append_from_file_descriptor(_pipe_fds[0], 1024));
            melon::rpc::ParseResult pr =
                    melon::rpc::policy::ParseHttpMessage(&buf, _socket.get(), false, nullptr);
            EXPECT_EQ(melon::rpc::PARSE_OK, pr.error());
            melon::rpc::policy::HttpContext *msg =
                    static_cast<melon::rpc::policy::HttpContext *>(pr.message());

            EXPECT_EQ(expect_code, msg->header().status_code());
            msg->Destroy();
        }

        void MakeH2EchoRequestBuf(melon::cord_buf *out, melon::rpc::Controller *cntl, int *h2_stream_id) {
            melon::cord_buf request_buf;
            test::EchoRequest req;
            req.set_message(EXP_REQUEST);
            cntl->http_request().set_method(melon::rpc::HTTP_METHOD_POST);
            melon::rpc::policy::SerializeHttpRequest(&request_buf, cntl, &req);
            ASSERT_FALSE(cntl->Failed());
            melon::rpc::policy::H2UnsentRequest *h2_req = melon::rpc::policy::H2UnsentRequest::New(cntl);
            cntl->_current_call.stream_user_data = h2_req;
            melon::rpc::SocketMessage *socket_message = nullptr;
            melon::rpc::policy::PackH2Request(nullptr, &socket_message, cntl->call_id().value,
                                              nullptr, cntl, request_buf, nullptr);
            melon::result_status st = socket_message->AppendAndDestroySelf(out, _h2_client_sock.get());
            ASSERT_TRUE(st.is_ok());
            *h2_stream_id = h2_req->_stream_id;
        }

        void MakeH2EchoResponseBuf(melon::cord_buf *out, int h2_stream_id) {
            melon::rpc::Controller cntl;
            test::EchoResponse res;
            res.set_message(EXP_RESPONSE);
            cntl.http_request().set_content_type("application/proto");
            {
                melon::cord_buf_as_zero_copy_output_stream wrapper(&cntl.response_attachment());
                EXPECT_TRUE(res.SerializeToZeroCopyStream(&wrapper));
            }
            melon::rpc::policy::H2UnsentResponse *h2_res = melon::rpc::policy::H2UnsentResponse::New(&cntl,
                                                                                                     h2_stream_id,
                                                                                                     false /*is grpc*/);
            melon::result_status st = h2_res->AppendAndDestroySelf(out, _h2_client_sock.get());
            ASSERT_TRUE(st);
        }

        int _pipe_fds[2];
        melon::rpc::SocketUniquePtr _socket;
        melon::rpc::SocketUniquePtr _h2_client_sock;
        melon::rpc::Server _server;

        MyEchoService _svc;
        MyAuthenticator _auth;
    };

    TEST_F(HttpTest, indenting_ostream) {
        std::ostringstream os1;
        melon::rpc::IndentingOStream is1(os1, 2);
        melon::rpc::IndentingOStream is2(is1, 2);
        os1 << "begin1\nhello" << std::endl << "world\nend1" << std::endl;
        is1 << "begin2\nhello" << std::endl << "world\nend2" << std::endl;
        is2 << "begin3\nhello" << std::endl << "world\nend3" << std::endl;
        ASSERT_EQ(
                "begin1\nhello\nworld\nend1\nbegin2\n  hello\n  world\n  end2\n"
                "  begin3\n    hello\n    world\n    end3\n",
                os1.str());
    }

    TEST_F(HttpTest, parse_http_address) {
        const std::string EXP_HOSTNAME = "www.baidu.com:9876";
        melon::base::end_point EXP_ENDPOINT;
        {
            std::string url = "https://" + EXP_HOSTNAME;
            EXPECT_TRUE(melon::rpc::policy::ParseHttpServerAddress(&EXP_ENDPOINT, url.c_str()));
        }
        {
            melon::base::end_point ep;
            std::string url = "http://" +
                              std::string(endpoint2str(EXP_ENDPOINT).c_str());
            EXPECT_TRUE(melon::rpc::policy::ParseHttpServerAddress(&ep, url.c_str()));
            EXPECT_EQ(EXP_ENDPOINT, ep);
        }
        {
            melon::base::end_point ep;
            std::string url = "https://" +
                              std::string(melon::base::ip2str(EXP_ENDPOINT.ip).c_str());
            EXPECT_TRUE(melon::rpc::policy::ParseHttpServerAddress(&ep, url.c_str()));
            EXPECT_EQ(EXP_ENDPOINT.ip, ep.ip);
            EXPECT_EQ(443, ep.port);
        }
        {
            melon::base::end_point ep;
            EXPECT_FALSE(melon::rpc::policy::ParseHttpServerAddress(&ep, "invalid_url"));
        }
        {
            melon::base::end_point ep;
            EXPECT_FALSE(melon::rpc::policy::ParseHttpServerAddress(
                    &ep, "https://no.such.machine:9090"));
        }
    }

    TEST_F(HttpTest, verify_request) {
        {
            melon::rpc::policy::HttpContext *msg =
                    MakePostRequestMessage("/EchoService/Echo");
            VerifyMessage(msg, false);
            msg->Destroy();
        }
        {
            melon::rpc::policy::HttpContext *msg =
                    MakePostProtoTextRequestMessage("/EchoService/Echo");
            VerifyMessage(msg, false);
            msg->Destroy();
        }
        {
            melon::rpc::policy::HttpContext *msg = MakeGetRequestMessage("/status");
            VerifyMessage(msg, true);
            msg->Destroy();
        }
        {
            melon::rpc::policy::HttpContext *msg =
                    MakePostRequestMessage("/EchoService/Echo");
            _socket->SetFailed();
            VerifyMessage(msg, false);
            msg->Destroy();
        }
    }

    TEST_F(HttpTest, process_request_failed_socket) {
        melon::rpc::policy::HttpContext *msg = MakePostRequestMessage("/EchoService/Echo");
        _socket->SetFailed();
        ProcessMessage(melon::rpc::policy::ProcessHttpRequest, msg, false);
        ASSERT_EQ(0ll, _server._nerror_var.get_value());
        CheckResponseCode(true, 0);
    }

    TEST_F(HttpTest, reject_get_to_pb_services_with_required_fields) {
        melon::rpc::policy::HttpContext *msg = MakeGetRequestMessage("/EchoService/Echo");
        _server._status = melon::rpc::Server::RUNNING;
        ProcessMessage(melon::rpc::policy::ProcessHttpRequest, msg, false);
        ASSERT_EQ(0ll, _server._nerror_var.get_value());
        const melon::rpc::Server::MethodProperty *mp =
                _server.FindMethodPropertyByFullName("test.EchoService.Echo");
        ASSERT_TRUE(mp);
        ASSERT_TRUE(mp->status);
        ASSERT_EQ(1ll, mp->status->_nerror_var.get_value());
        CheckResponseCode(false, melon::rpc::HTTP_STATUS_BAD_REQUEST);
    }

    TEST_F(HttpTest, process_request_logoff) {
        melon::rpc::policy::HttpContext *msg = MakePostRequestMessage("/EchoService/Echo");
        _server._status = melon::rpc::Server::READY;
        ProcessMessage(melon::rpc::policy::ProcessHttpRequest, msg, false);
        ASSERT_EQ(1ll, _server._nerror_var.get_value());
        CheckResponseCode(false, melon::rpc::HTTP_STATUS_SERVICE_UNAVAILABLE);
    }

    TEST_F(HttpTest, process_request_wrong_method) {
        melon::rpc::policy::HttpContext *msg = MakePostRequestMessage("/NO_SUCH_METHOD");
        ProcessMessage(melon::rpc::policy::ProcessHttpRequest, msg, false);
        ASSERT_EQ(1ll, _server._nerror_var.get_value());
        CheckResponseCode(false, melon::rpc::HTTP_STATUS_NOT_FOUND);
    }

    TEST_F(HttpTest, process_response_after_eof) {
        test::EchoResponse res;
        melon::rpc::Controller cntl;
        cntl._response = &res;
        melon::rpc::policy::HttpContext *msg =
                MakeResponseMessage(melon::rpc::HTTP_STATUS_OK);
        _socket->set_correlation_id(cntl.call_id().value);
        ProcessMessage(melon::rpc::policy::ProcessHttpResponse, msg, true);
        ASSERT_EQ(EXP_RESPONSE, res.message());
        ASSERT_TRUE(_socket->Failed());
    }

    TEST_F(HttpTest, process_response_error_code) {
        {
            melon::rpc::Controller cntl;
            _socket->set_correlation_id(cntl.call_id().value);
            melon::rpc::policy::HttpContext *msg =
                    MakeResponseMessage(melon::rpc::HTTP_STATUS_CONTINUE);
            ProcessMessage(melon::rpc::policy::ProcessHttpResponse, msg, false);
            ASSERT_EQ(melon::rpc::EHTTP, cntl.ErrorCode());
            ASSERT_EQ(melon::rpc::HTTP_STATUS_CONTINUE, cntl.http_response().status_code());
        }
        {
            melon::rpc::Controller cntl;
            _socket->set_correlation_id(cntl.call_id().value);
            melon::rpc::policy::HttpContext *msg =
                    MakeResponseMessage(melon::rpc::HTTP_STATUS_TEMPORARY_REDIRECT);
            ProcessMessage(melon::rpc::policy::ProcessHttpResponse, msg, false);
            ASSERT_EQ(melon::rpc::EHTTP, cntl.ErrorCode());
            ASSERT_EQ(melon::rpc::HTTP_STATUS_TEMPORARY_REDIRECT,
                      cntl.http_response().status_code());
        }
        {
            melon::rpc::Controller cntl;
            _socket->set_correlation_id(cntl.call_id().value);
            melon::rpc::policy::HttpContext *msg =
                    MakeResponseMessage(melon::rpc::HTTP_STATUS_BAD_REQUEST);
            ProcessMessage(melon::rpc::policy::ProcessHttpResponse, msg, false);
            ASSERT_EQ(melon::rpc::EHTTP, cntl.ErrorCode());
            ASSERT_EQ(melon::rpc::HTTP_STATUS_BAD_REQUEST,
                      cntl.http_response().status_code());
        }
        {
            melon::rpc::Controller cntl;
            _socket->set_correlation_id(cntl.call_id().value);
            melon::rpc::policy::HttpContext *msg = MakeResponseMessage(12345);
            ProcessMessage(melon::rpc::policy::ProcessHttpResponse, msg, false);
            ASSERT_EQ(melon::rpc::EHTTP, cntl.ErrorCode());
            ASSERT_EQ(12345, cntl.http_response().status_code());
        }
    }

    TEST_F(HttpTest, complete_flow) {
        melon::cord_buf request_buf;
        melon::cord_buf total_buf;
        melon::rpc::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        cntl._response = &res;
        cntl._connection_type = melon::rpc::CONNECTION_TYPE_SHORT;
        cntl._method = test::EchoService::descriptor()->method(0);
        ASSERT_EQ(0, melon::rpc::Socket::Address(_socket->id(), &cntl._current_call.sending_sock));

        // Send request
        req.set_message(EXP_REQUEST);
        melon::rpc::policy::SerializeHttpRequest(&request_buf, &cntl, &req);
        ASSERT_FALSE(cntl.Failed());
        melon::rpc::policy::PackHttpRequest(
                &total_buf, nullptr, cntl.call_id().value,
                cntl._method, &cntl, request_buf, &_auth);
        ASSERT_FALSE(cntl.Failed());

        // Verify and handle request
        melon::rpc::ParseResult req_pr =
                melon::rpc::policy::ParseHttpMessage(&total_buf, _socket.get(), false, nullptr);
        ASSERT_EQ(melon::rpc::PARSE_OK, req_pr.error());
        melon::rpc::InputMessageBase *req_msg = req_pr.message();
        VerifyMessage(req_msg, true);
        ProcessMessage(melon::rpc::policy::ProcessHttpRequest, req_msg, false);

        // Read response from pipe
        melon::IOPortal response_buf;
        response_buf.append_from_file_descriptor(_pipe_fds[0], 1024);
        melon::rpc::ParseResult res_pr =
                melon::rpc::policy::ParseHttpMessage(&response_buf, _socket.get(), false, nullptr);
        ASSERT_EQ(melon::rpc::PARSE_OK, res_pr.error());
        melon::rpc::InputMessageBase *res_msg = res_pr.message();
        ProcessMessage(melon::rpc::policy::ProcessHttpResponse, res_msg, false);

        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        ASSERT_EQ(EXP_RESPONSE, res.message());
    }

    TEST_F(HttpTest, chunked_uploading) {
        const int port = 8923;
        melon::rpc::Server server;
        EXPECT_EQ(0, server.AddService(&_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));

        // Send request via curl using chunked encoding
        const std::string req = "{\"message\":\"hello\"}";
        const std::string res_fname = "curl.out";
        std::string cmd;
        melon::string_printf(&cmd, "curl -X POST -d '%s' -H 'Transfer-Encoding:chunked' "
                                   "-H 'Content-Type:application/json' -o %s "
                                   "http://localhost:%d/EchoService/Echo",
                             req.c_str(), res_fname.c_str(), port);
        ASSERT_EQ(0, system(cmd.c_str()));

        // Check response
        const std::string exp_res = "{\"message\":\"world\"}";
        melon::sequential_read_file file;
        file.open(res_fname);
        std::string content;
        ASSERT_TRUE(file.read(&content));
        EXPECT_EQ(exp_res, content);
    }

    enum DonePlace {
        DONE_BEFORE_CREATE_PA = 0,
        DONE_AFTER_CREATE_PA_BEFORE_DESTROY_PA,
        DONE_AFTER_DESTROY_PA,
    };
// For writing into PA.
    const char PA_DATA[] = "abcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()_=-+";
    const size_t PA_DATA_LEN = sizeof(PA_DATA) - 1/*not count the ending zero*/;

    static void CopyPAPrefixedWithSeqNo(char *buf, uint64_t seq_no) {
        memcpy(buf, PA_DATA, PA_DATA_LEN);
        *(uint64_t *) buf = seq_no;
    }

    class DownloadServiceImpl : public ::test::DownloadService {
    public:
        DownloadServiceImpl(DonePlace done_place = DONE_BEFORE_CREATE_PA,
                            size_t num_repeat = 1)
                : _done_place(done_place), _nrep(num_repeat), _nwritten(0), _ever_full(false), _last_errno(0) {}

        void Download(::google::protobuf::RpcController *cntl_base,
                      const ::test::HttpRequest *,
                      ::test::HttpResponse *,
                      ::google::protobuf::Closure *done) {
            melon::rpc::ClosureGuard done_guard(done);
            melon::rpc::Controller *cntl =
                    static_cast<melon::rpc::Controller *>(cntl_base);
            cntl->http_response().set_content_type("text/plain");
            melon::rpc::StopStyle stop_style = (_nrep == std::numeric_limits<size_t>::max()
                                                ? melon::rpc::FORCE_STOP : melon::rpc::WAIT_FOR_STOP);
            melon::container::intrusive_ptr<melon::rpc::ProgressiveAttachment> pa
                    = cntl->CreateProgressiveAttachment(stop_style);
            if (pa == nullptr) {
                cntl->SetFailed("The socket was just failed");
                return;
            }
            if (_done_place == DONE_BEFORE_CREATE_PA) {
                done_guard.reset(nullptr);
            }
            ASSERT_GT(PA_DATA_LEN, 8u);  // long enough to hold a 64-bit decimal.
            char buf[PA_DATA_LEN];
            for (size_t c = 0; c < _nrep;) {
                CopyPAPrefixedWithSeqNo(buf, c);
                if (pa->Write(buf, sizeof(buf)) != 0) {
                    if (errno == melon::rpc::EOVERCROWDED) {
                        MELON_LOG_EVERY_SECOND(INFO) << "full pa=" << pa.get();
                        _ever_full = true;
                        melon::fiber_sleep_for(10000);
                        continue;
                    } else {
                        _last_errno = errno;
                        break;
                    }
                } else {
                    _nwritten += PA_DATA_LEN;
                }
                ++c;
            }
            if (_done_place == DONE_AFTER_CREATE_PA_BEFORE_DESTROY_PA) {
                done_guard.reset(nullptr);
            }
            MELON_LOG(INFO) << "Destroy pa=" << pa.get();
            pa.reset(nullptr);
            if (_done_place == DONE_AFTER_DESTROY_PA) {
                done_guard.reset(nullptr);
            }
        }

        void DownloadFailed(::google::protobuf::RpcController *cntl_base,
                            const ::test::HttpRequest *,
                            ::test::HttpResponse *,
                            ::google::protobuf::Closure *done) {
            melon::rpc::ClosureGuard done_guard(done);
            melon::rpc::Controller *cntl =
                    static_cast<melon::rpc::Controller *>(cntl_base);
            cntl->http_response().set_content_type("text/plain");
            melon::rpc::StopStyle stop_style = (_nrep == std::numeric_limits<size_t>::max()
                                                ? melon::rpc::FORCE_STOP : melon::rpc::WAIT_FOR_STOP);
            melon::container::intrusive_ptr<melon::rpc::ProgressiveAttachment> pa
                    = cntl->CreateProgressiveAttachment(stop_style);
            if (pa == nullptr) {
                cntl->SetFailed("The socket was just failed");
                return;
            }
            char buf[PA_DATA_LEN];
            while (true) {
                if (pa->Write(buf, sizeof(buf)) != 0) {
                    if (errno == melon::rpc::EOVERCROWDED) {
                        MELON_LOG_EVERY_SECOND(INFO) << "full pa=" << pa.get();
                        melon::fiber_sleep_for(10000);
                        continue;
                    } else {
                        _last_errno = errno;
                        break;
                    }
                }
                break;
            }
            // The remote client will not receive the data written to the
            // progressive attachment when the controller failed.
            cntl->SetFailed("Intentionally set controller failed");
            done_guard.reset(nullptr);

            // Return value of Write after controller has failed should
            // be less than zero.
            MELON_CHECK_LT(pa->Write(buf, sizeof(buf)), 0);
            MELON_CHECK_EQ(errno, ECANCELED);
        }

        void set_done_place(DonePlace done_place) { _done_place = done_place; }

        size_t written_bytes() const { return _nwritten; }

        bool ever_full() const { return _ever_full; }

        int last_errno() const { return _last_errno; }

    private:
        DonePlace _done_place;
        size_t _nrep;
        size_t _nwritten;
        bool _ever_full;
        int _last_errno;
    };

    TEST_F(HttpTest, read_chunked_response_normally) {
        const int port = 8923;
        melon::rpc::Server server;
        DownloadServiceImpl svc;
        EXPECT_EQ(0, server.AddService(&svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));

        for (int i = 0; i < 3; ++i) {
            svc.set_done_place((DonePlace) i);
            melon::rpc::Channel channel;
            melon::rpc::ChannelOptions options;
            options.protocol = melon::rpc::PROTOCOL_HTTP;
            ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));
            melon::rpc::Controller cntl;
            cntl.http_request().uri() = "/DownloadService/Download";
            channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
            ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();

            std::string expected(PA_DATA_LEN, 0);
            CopyPAPrefixedWithSeqNo(&expected[0], 0);
            ASSERT_EQ(expected, cntl.response_attachment());
        }
    }

    TEST_F(HttpTest, read_failed_chunked_response) {
        const int port = 8923;
        melon::rpc::Server server;
        DownloadServiceImpl svc;
        EXPECT_EQ(0, server.AddService(&svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));

        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions options;
        options.protocol = melon::rpc::PROTOCOL_HTTP;
        ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));

        melon::rpc::Controller cntl;
        cntl.http_request().uri() = "/DownloadService/DownloadFailed";
        cntl.response_will_be_read_progressively();
        channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
        EXPECT_TRUE(cntl.response_attachment().empty());
        ASSERT_TRUE(cntl.Failed());
        ASSERT_NE(cntl.ErrorText().find("HTTP/1.1 500 Internal Server Error"),
                  std::string::npos) << cntl.ErrorText();
        ASSERT_NE(cntl.ErrorText().find("Intentionally set controller failed"),
                  std::string::npos) << cntl.ErrorText();
        ASSERT_EQ(0, svc.last_errno());
    }

    class ReadBody : public melon::rpc::ProgressiveReader,
                     public melon::rpc::SharedObject {
    public:
        ReadBody()
                : _nread(0), _ncount(0), _destroyed(false) {
            melon::container::intrusive_ptr<ReadBody>(this).detach(); // ref
        }

        melon::result_status OnReadOnePart(const void *data, size_t length) {
            _nread += length;
            while (length > 0) {
                size_t nappend = std::min(_buf.size() + length, PA_DATA_LEN) - _buf.size();
                _buf.append((const char *) data, nappend);
                data = (const char *) data + nappend;
                length -= nappend;
                if (_buf.size() >= PA_DATA_LEN) {
                    EXPECT_EQ(PA_DATA_LEN, _buf.size());
                    char expected[PA_DATA_LEN];
                    CopyPAPrefixedWithSeqNo(expected, _ncount++);
                    EXPECT_EQ(0, memcmp(expected, _buf.data(), PA_DATA_LEN))
                                        << "ncount=" << _ncount;
                    _buf.clear();
                }
            }
            return melon::result_status::success();
        }

        void OnEndOfMessage(const melon::result_status &st) {
            melon::container::intrusive_ptr<ReadBody>(this, false); // deref
            ASSERT_LT(_buf.size(), PA_DATA_LEN);
            ASSERT_EQ(0, memcmp(_buf.data(), PA_DATA, _buf.size()));
            _destroyed = true;
            _destroying_st = st;
            MELON_LOG(INFO) << "Destroy ReadBody=" << this << ", " << st;
        }

        bool destroyed() const { return _destroyed; }

        const melon::result_status &destroying_status() const { return _destroying_st; }

        size_t read_bytes() const { return _nread; }

    private:
        std::string _buf;
        size_t _nread;
        size_t _ncount;
        bool _destroyed;
        melon::result_status _destroying_st;
    };

    static const int GENERAL_DELAY_US = 300000; // 0.3s

    TEST_F(HttpTest, read_long_body_progressively) {
        melon::container::intrusive_ptr<ReadBody> reader;
        {
            const int port = 8923;
            melon::rpc::Server server;
            DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                    std::numeric_limits<size_t>::max());
            EXPECT_EQ(0, server.AddService(&svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
            EXPECT_EQ(0, server.Start(port, nullptr));
            {
                melon::rpc::Channel channel;
                melon::rpc::ChannelOptions options;
                options.protocol = melon::rpc::PROTOCOL_HTTP;
                ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));
                {
                    melon::rpc::Controller cntl;
                    cntl.response_will_be_read_progressively();
                    cntl.http_request().uri() = "/DownloadService/Download";
                    channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
                    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
                    ASSERT_TRUE(cntl.response_attachment().empty());
                    reader.reset(new ReadBody);
                    cntl.ReadProgressiveAttachmentBy(reader.get());
                    size_t last_read = 0;
                    for (size_t i = 0; i < 3; ++i) {
                        sleep(1);
                        size_t current_read = reader->read_bytes();
                        MELON_LOG(INFO) << "read=" << current_read - last_read
                                        << " total=" << current_read;
                        last_read = current_read;
                    }
                    // Read something in past N seconds.
                    ASSERT_GT(last_read, (size_t) 100000);
                }
                // the socket still holds a ref.
                ASSERT_FALSE(reader->destroyed());
            }
            // Wait for recycling of the main socket.
            usleep(GENERAL_DELAY_US);
            // even if the main socket is recycled, the pooled socket for
            // receiving data is not affected.
            ASSERT_FALSE(reader->destroyed());
        }
        // Wait for close of the connection due to server's stopping.
        usleep(GENERAL_DELAY_US);
        ASSERT_TRUE(reader->destroyed());
        ASSERT_EQ(ECONNRESET, reader->destroying_status().error_code());
    }

    TEST_F(HttpTest, read_short_body_progressively) {
        melon::container::intrusive_ptr<ReadBody> reader;
        const int port = 8923;
        melon::rpc::Server server;
        const int NREP = 10000;
        DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA, NREP);
        EXPECT_EQ(0, server.AddService(&svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));
        {
            melon::rpc::Channel channel;
            melon::rpc::ChannelOptions options;
            options.protocol = melon::rpc::PROTOCOL_HTTP;
            ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));
            {
                melon::rpc::Controller cntl;
                cntl.response_will_be_read_progressively();
                cntl.http_request().uri() = "/DownloadService/Download";
                channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
                ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
                ASSERT_TRUE(cntl.response_attachment().empty());
                reader.reset(new ReadBody);
                cntl.ReadProgressiveAttachmentBy(reader.get());
                size_t last_read = 0;
                for (size_t i = 0; i < 3; ++i) {
                    sleep(1);
                    size_t current_read = reader->read_bytes();
                    MELON_LOG(INFO) << "read=" << current_read - last_read
                                    << " total=" << current_read;
                    last_read = current_read;
                }
                ASSERT_EQ(NREP * PA_DATA_LEN, svc.written_bytes());
                ASSERT_EQ(NREP * PA_DATA_LEN, last_read);
            }
            ASSERT_TRUE(reader->destroyed());
            ASSERT_EQ(0, reader->destroying_status().error_code());
        }
    }

    TEST_F(HttpTest, read_progressively_after_cntl_destroys) {
        melon::container::intrusive_ptr<ReadBody> reader;
        {
            const int port = 8923;
            melon::rpc::Server server;
            DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                    std::numeric_limits<size_t>::max());
            EXPECT_EQ(0, server.AddService(&svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
            EXPECT_EQ(0, server.Start(port, nullptr));
            {
                melon::rpc::Channel channel;
                melon::rpc::ChannelOptions options;
                options.protocol = melon::rpc::PROTOCOL_HTTP;
                ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));
                {
                    melon::rpc::Controller cntl;
                    cntl.response_will_be_read_progressively();
                    cntl.http_request().uri() = "/DownloadService/Download";
                    channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
                    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
                    ASSERT_TRUE(cntl.response_attachment().empty());
                    reader.reset(new ReadBody);
                    cntl.ReadProgressiveAttachmentBy(reader.get());
                }
                size_t last_read = 0;
                for (size_t i = 0; i < 3; ++i) {
                    sleep(1);
                    size_t current_read = reader->read_bytes();
                    MELON_LOG(INFO) << "read=" << current_read - last_read
                                    << " total=" << current_read;
                    last_read = current_read;
                }
                // Read something in past N seconds.
                ASSERT_GT(last_read, (size_t) 100000);
                ASSERT_FALSE(reader->destroyed());
            }
            // Wait for recycling of the main socket.
            usleep(GENERAL_DELAY_US);
            ASSERT_FALSE(reader->destroyed());
        }
        // Wait for close of the connection due to server's stopping.
        usleep(GENERAL_DELAY_US);
        ASSERT_TRUE(reader->destroyed());
        ASSERT_EQ(ECONNRESET, reader->destroying_status().error_code());
    }

    TEST_F(HttpTest, read_progressively_after_long_delay) {
        melon::container::intrusive_ptr<ReadBody> reader;
        {
            const int port = 8923;
            melon::rpc::Server server;
            DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                    std::numeric_limits<size_t>::max());
            EXPECT_EQ(0, server.AddService(&svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
            EXPECT_EQ(0, server.Start(port, nullptr));
            {
                melon::rpc::Channel channel;
                melon::rpc::ChannelOptions options;
                options.protocol = melon::rpc::PROTOCOL_HTTP;
                ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));
                {
                    melon::rpc::Controller cntl;
                    cntl.response_will_be_read_progressively();
                    cntl.http_request().uri() = "/DownloadService/Download";
                    channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
                    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
                    ASSERT_TRUE(cntl.response_attachment().empty());
                    MELON_LOG(INFO) << "Sleep 3 seconds to make PA at server-side full";
                    sleep(3);
                    EXPECT_TRUE(svc.ever_full());
                    ASSERT_EQ(0, svc.last_errno());
                    reader.reset(new ReadBody);
                    cntl.ReadProgressiveAttachmentBy(reader.get());
                    size_t last_read = 0;
                    for (size_t i = 0; i < 3; ++i) {
                        sleep(1);
                        size_t current_read = reader->read_bytes();
                        MELON_LOG(INFO) << "read=" << current_read - last_read
                                        << " total=" << current_read;
                        last_read = current_read;
                    }
                    // Read something in past N seconds.
                    ASSERT_GT(last_read, (size_t) 100000);
                }
                ASSERT_FALSE(reader->destroyed());
            }
            // Wait for recycling of the main socket.
            usleep(GENERAL_DELAY_US);
            ASSERT_FALSE(reader->destroyed());
        }
        // Wait for close of the connection due to server's stopping.
        usleep(GENERAL_DELAY_US);
        ASSERT_TRUE(reader->destroyed());
        ASSERT_EQ(ECONNRESET, reader->destroying_status().error_code());
    }

    TEST_F(HttpTest, skip_progressive_reading) {
        const int port = 8923;
        melon::rpc::Server server;
        DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                std::numeric_limits<size_t>::max());
        EXPECT_EQ(0, server.AddService(&svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));
        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions options;
        options.protocol = melon::rpc::PROTOCOL_HTTP;
        ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));
        {
            melon::rpc::Controller cntl;
            cntl.response_will_be_read_progressively();
            cntl.http_request().uri() = "/DownloadService/Download";
            channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
            ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
            ASSERT_TRUE(cntl.response_attachment().empty());
        }
        const size_t old_written_bytes = svc.written_bytes();
        MELON_LOG(INFO) << "Sleep 3 seconds after destroy of Controller";
        sleep(3);
        const size_t new_written_bytes = svc.written_bytes();
        ASSERT_EQ(0, svc.last_errno());
        MELON_LOG(INFO) << "Server still wrote " << new_written_bytes - old_written_bytes;
        // The server side still wrote things.
        ASSERT_GT(new_written_bytes - old_written_bytes, (size_t) 100000);
    }

    class AlwaysFailRead : public melon::rpc::ProgressiveReader {
    public:
        // @ProgressiveReader
        melon::result_status OnReadOnePart(const void * /*data*/, size_t /*length*/) {
            return melon::result_status(-1, "intended fail at {}:{}", __FILE__, __LINE__);
        }

        void OnEndOfMessage(const melon::result_status &st) {
            MELON_LOG(INFO) << "Destroy " << this << ": " << st;
            delete this;
        }
    };

    TEST_F(HttpTest, failed_on_read_one_part) {
        const int port = 8923;
        melon::rpc::Server server;
        DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                std::numeric_limits<size_t>::max());
        EXPECT_EQ(0, server.AddService(&svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));
        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions options;
        options.protocol = melon::rpc::PROTOCOL_HTTP;
        ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));
        {
            melon::rpc::Controller cntl;
            cntl.response_will_be_read_progressively();
            cntl.http_request().uri() = "/DownloadService/Download";
            channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
            ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
            ASSERT_TRUE(cntl.response_attachment().empty());
            cntl.ReadProgressiveAttachmentBy(new AlwaysFailRead);
        }
        MELON_LOG(INFO) << "Sleep 1 second";
        sleep(1);
        ASSERT_NE(0, svc.last_errno());
    }

    TEST_F(HttpTest, broken_socket_stops_progressive_reading) {
        melon::container::intrusive_ptr<ReadBody> reader;
        const int port = 8923;
        melon::rpc::Server server;
        DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                std::numeric_limits<size_t>::max());
        EXPECT_EQ(0, server.AddService(&svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));

        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions options;
        options.protocol = melon::rpc::PROTOCOL_HTTP;
        ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));
        {
            melon::rpc::Controller cntl;
            cntl.response_will_be_read_progressively();
            cntl.http_request().uri() = "/DownloadService/Download";
            channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
            ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
            ASSERT_TRUE(cntl.response_attachment().empty());
            reader.reset(new ReadBody);
            cntl.ReadProgressiveAttachmentBy(reader.get());
            size_t last_read = 0;
            for (size_t i = 0; i < 3; ++i) {
                sleep(1);
                size_t current_read = reader->read_bytes();
                MELON_LOG(INFO) << "read=" << current_read - last_read
                                << " total=" << current_read;
                last_read = current_read;
            }
            // Read something in past N seconds.
            ASSERT_GT(last_read, (size_t) 100000);
        }
        // the socket still holds a ref.
        ASSERT_FALSE(reader->destroyed());
        MELON_LOG(INFO) << "Stopping the server";
        server.Stop(0);
        server.Join();

        // Wait for error reporting from the socket.
        usleep(GENERAL_DELAY_US);
        ASSERT_TRUE(reader->destroyed());
        ASSERT_EQ(ECONNRESET, reader->destroying_status().error_code());
    }

    TEST_F(HttpTest, http2_sanity) {
        const int port = 8923;
        melon::rpc::Server server;
        EXPECT_EQ(0, server.AddService(&_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));

        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions options;
        options.protocol = "h2";
        ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));

        // Check that the first request with size larger than the default window can
        // be sent out, when remote settings are not received.
        melon::rpc::Controller cntl;
        test::EchoRequest big_req;
        test::EchoResponse res;
        std::string message(2 * 1024 * 1024 /* 2M */, 'x');
        big_req.set_message(message);
        cntl.http_request().set_method(melon::rpc::HTTP_METHOD_POST);
        cntl.http_request().uri() = "/EchoService/Echo";
        channel.CallMethod(nullptr, &cntl, &big_req, &res, nullptr);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_EQ(EXP_RESPONSE, res.message());

        // socket replacement when streamId runs out, the initial streamId is a special
        // value set in ctor of H2Context so that the number 15000 is enough to run out
        // of stream.
        test::EchoRequest req;
        req.set_message(EXP_REQUEST);
        for (int i = 0; i < 15000; ++i) {
            melon::rpc::Controller cntl;
            cntl.http_request().set_content_type("application/json");
            cntl.http_request().set_method(melon::rpc::HTTP_METHOD_POST);
            cntl.http_request().uri() = "/EchoService/Echo";
            channel.CallMethod(nullptr, &cntl, &req, &res, nullptr);
            ASSERT_FALSE(cntl.Failed());
            ASSERT_EQ(EXP_RESPONSE, res.message());
        }

        // check connection window size
        melon::rpc::SocketUniquePtr main_ptr;
        melon::rpc::SocketUniquePtr agent_ptr;
        EXPECT_EQ(melon::rpc::Socket::Address(channel._server_id, &main_ptr), 0);
        EXPECT_EQ(main_ptr->GetAgentSocket(&agent_ptr, nullptr), 0);
        melon::rpc::policy::H2Context *ctx = static_cast<melon::rpc::policy::H2Context *>(agent_ptr->parsing_context());
        ASSERT_GT(ctx->_remote_window_left.load(std::memory_order_relaxed),
                  melon::rpc::H2Settings::DEFAULT_INITIAL_WINDOW_SIZE / 2);
    }

    TEST_F(HttpTest, http2_ping) {
        // This test injects PING frames before and after header and data.
        melon::rpc::Controller cntl;

        // Prepare request
        melon::cord_buf req_out;
        int h2_stream_id = 0;
        MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);
        // Prepare response
        melon::cord_buf res_out;
        char pingbuf[9 /*FRAME_HEAD_SIZE*/ + 8 /*Opaque Data*/];
        melon::rpc::policy::SerializeFrameHead(pingbuf, 8, melon::rpc::policy::H2_FRAME_PING, 0, 0);
        // insert ping before header and data
        res_out.append(pingbuf, sizeof(pingbuf));
        MakeH2EchoResponseBuf(&res_out, h2_stream_id);
        // insert ping after header and data
        res_out.append(pingbuf, sizeof(pingbuf));
        // parse response
        melon::rpc::ParseResult res_pr =
                melon::rpc::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, nullptr);
        ASSERT_TRUE(res_pr.is_ok());
        // process response
        ProcessMessage(melon::rpc::policy::ProcessHttpResponse, res_pr.message(), false);
        ASSERT_FALSE(cntl.Failed());
    }

    inline void SaveUint32(void *out, uint32_t v) {
        uint8_t *p = (uint8_t *) out;
        p[0] = (v >> 24) & 0xFF;
        p[1] = (v >> 16) & 0xFF;
        p[2] = (v >> 8) & 0xFF;
        p[3] = v & 0xFF;
    }

    TEST_F(HttpTest, http2_rst_before_header) {
        melon::rpc::Controller cntl;
        // Prepare request
        melon::cord_buf req_out;
        int h2_stream_id = 0;
        MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);
        // Prepare response
        melon::cord_buf res_out;
        char rstbuf[9 /*FRAME_HEAD_SIZE*/ + 4];
        melon::rpc::policy::SerializeFrameHead(rstbuf, 4, melon::rpc::policy::H2_FRAME_RST_STREAM, 0, h2_stream_id);
        SaveUint32(rstbuf + 9, melon::rpc::H2_INTERNAL_ERROR);
        res_out.append(rstbuf, sizeof(rstbuf));
        MakeH2EchoResponseBuf(&res_out, h2_stream_id);
        // parse response
        melon::rpc::ParseResult res_pr =
                melon::rpc::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, nullptr);
        ASSERT_TRUE(res_pr.is_ok());
        // process response
        ProcessMessage(melon::rpc::policy::ProcessHttpResponse, res_pr.message(), false);
        ASSERT_TRUE(cntl.Failed());
        ASSERT_TRUE(cntl.ErrorCode() == melon::rpc::EHTTP);
        ASSERT_TRUE(cntl.http_response().status_code() == melon::rpc::HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }

    TEST_F(HttpTest, http2_rst_after_header_and_data) {
        melon::rpc::Controller cntl;
        // Prepare request
        melon::cord_buf req_out;
        int h2_stream_id = 0;
        MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);
        // Prepare response
        melon::cord_buf res_out;
        MakeH2EchoResponseBuf(&res_out, h2_stream_id);
        char rstbuf[9 /*FRAME_HEAD_SIZE*/ + 4];
        melon::rpc::policy::SerializeFrameHead(rstbuf, 4, melon::rpc::policy::H2_FRAME_RST_STREAM, 0, h2_stream_id);
        SaveUint32(rstbuf + 9, melon::rpc::H2_INTERNAL_ERROR);
        res_out.append(rstbuf, sizeof(rstbuf));
        // parse response
        melon::rpc::ParseResult res_pr =
                melon::rpc::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, nullptr);
        ASSERT_TRUE(res_pr.is_ok());
        // process response
        ProcessMessage(melon::rpc::policy::ProcessHttpResponse, res_pr.message(), false);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_TRUE(cntl.http_response().status_code() == melon::rpc::HTTP_STATUS_OK);
    }

    TEST_F(HttpTest, http2_window_used_up) {
        melon::rpc::Controller cntl;
        melon::cord_buf request_buf;
        test::EchoRequest req;
        // longer message to trigger using up window size sooner
        req.set_message("FLOW_CONTROL_FLOW_CONTROL");
        cntl.http_request().set_method(melon::rpc::HTTP_METHOD_POST);
        cntl.http_request().set_content_type("application/proto");
        melon::rpc::policy::SerializeHttpRequest(&request_buf, &cntl, &req);

        char settingsbuf[melon::rpc::policy::FRAME_HEAD_SIZE + 36];
        melon::rpc::H2Settings h2_settings;
        const size_t nb = melon::rpc::policy::SerializeH2Settings(h2_settings,
                                                                  settingsbuf + melon::rpc::policy::FRAME_HEAD_SIZE);
        melon::rpc::policy::SerializeFrameHead(settingsbuf, nb, melon::rpc::policy::H2_FRAME_SETTINGS, 0, 0);
        melon::cord_buf buf;
        buf.append(settingsbuf, melon::rpc::policy::FRAME_HEAD_SIZE + nb);
        melon::rpc::policy::ParseH2Message(&buf, _h2_client_sock.get(), false, nullptr);

        int nsuc = melon::rpc::H2Settings::DEFAULT_INITIAL_WINDOW_SIZE / cntl.request_attachment().size();
        for (int i = 0; i <= nsuc; i++) {
            melon::rpc::policy::H2UnsentRequest *h2_req = melon::rpc::policy::H2UnsentRequest::New(&cntl);
            cntl._current_call.stream_user_data = h2_req;
            melon::rpc::SocketMessage *socket_message = nullptr;
            melon::rpc::policy::PackH2Request(nullptr, &socket_message, cntl.call_id().value,
                                              nullptr, &cntl, request_buf, nullptr);
            melon::cord_buf dummy;
            melon::result_status st = socket_message->AppendAndDestroySelf(&dummy, _h2_client_sock.get());
            if (i == nsuc) {
                // the last message should fail according to flow control policy.
                ASSERT_FALSE(st);
                ASSERT_TRUE(st.error_code() == melon::rpc::ELIMIT);
                ASSERT_TRUE(melon::starts_with(st.error_data(), "remote_window_left is not enough"));
            } else {
                ASSERT_TRUE(st);
            }
            h2_req->DestroyStreamUserData(_h2_client_sock, &cntl, 0, false);
        }
    }

    TEST_F(HttpTest, http2_settings) {
        char settingsbuf[melon::rpc::policy::FRAME_HEAD_SIZE + 36];
        melon::rpc::H2Settings h2_settings;
        h2_settings.header_table_size = 8192;
        h2_settings.max_concurrent_streams = 1024;
        h2_settings.stream_window_size = (1u << 29) - 1;
        const size_t nb = melon::rpc::policy::SerializeH2Settings(h2_settings,
                                                                  settingsbuf + melon::rpc::policy::FRAME_HEAD_SIZE);
        melon::rpc::policy::SerializeFrameHead(settingsbuf, nb, melon::rpc::policy::H2_FRAME_SETTINGS, 0, 0);
        melon::cord_buf buf;
        buf.append(settingsbuf, melon::rpc::policy::FRAME_HEAD_SIZE + nb);

        melon::rpc::policy::H2Context *ctx = new melon::rpc::policy::H2Context(_socket.get(), nullptr);
        MELON_CHECK_EQ(ctx->Init(), 0);
        _socket->initialize_parsing_context(&ctx);
        ctx->_conn_state = melon::rpc::policy::H2_CONNECTION_READY;
        // parse settings
        melon::rpc::policy::ParseH2Message(&buf, _socket.get(), false, nullptr);

        melon::IOPortal response_buf;
        MELON_CHECK_EQ(response_buf.append_from_file_descriptor(_pipe_fds[0], 1024),
                       (ssize_t) melon::rpc::policy::FRAME_HEAD_SIZE);
        melon::rpc::policy::H2FrameHead frame_head;
        melon::cord_buf_bytes_iterator it(response_buf);
        ctx->ConsumeFrameHead(it, &frame_head);
        MELON_CHECK_EQ(frame_head.type, melon::rpc::policy::H2_FRAME_SETTINGS);
        MELON_CHECK_EQ(frame_head.flags, 0x01 /* H2_FLAGS_ACK */);
        MELON_CHECK_EQ(frame_head.stream_id, 0);
        ASSERT_TRUE(ctx->_remote_settings.header_table_size == 8192);
        ASSERT_TRUE(ctx->_remote_settings.max_concurrent_streams == 1024);
        ASSERT_TRUE(ctx->_remote_settings.stream_window_size == (1u << 29) - 1);
    }

    TEST_F(HttpTest, http2_invalid_settings) {
        {
            melon::rpc::Server server;
            melon::rpc::ServerOptions options;
            options.h2_settings.stream_window_size = melon::rpc::H2Settings::MAX_WINDOW_SIZE + 1;
            ASSERT_EQ(-1, server.Start("127.0.0.1:8924", &options));
        }
        {
            melon::rpc::Server server;
            melon::rpc::ServerOptions options;
            options.h2_settings.max_frame_size =
                    melon::rpc::H2Settings::DEFAULT_MAX_FRAME_SIZE - 1;
            ASSERT_EQ(-1, server.Start("127.0.0.1:8924", &options));
        }
        {
            melon::rpc::Server server;
            melon::rpc::ServerOptions options;
            options.h2_settings.max_frame_size =
                    melon::rpc::H2Settings::MAX_OF_MAX_FRAME_SIZE + 1;
            ASSERT_EQ(-1, server.Start("127.0.0.1:8924", &options));
        }
    }

    TEST_F(HttpTest, http2_not_closing_socket_when_rpc_timeout) {
        const int port = 8923;
        melon::rpc::Server server;
        EXPECT_EQ(0, server.AddService(&_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));
        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions options;
        options.protocol = "h2";
        ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));

        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(EXP_REQUEST);
        {
            // make a successful call to create the connection first
            melon::rpc::Controller cntl;
            cntl.http_request().set_method(melon::rpc::HTTP_METHOD_POST);
            cntl.http_request().uri() = "/EchoService/Echo";
            channel.CallMethod(nullptr, &cntl, &req, &res, nullptr);
            ASSERT_FALSE(cntl.Failed());
            ASSERT_EQ(EXP_RESPONSE, res.message());
        }

        melon::rpc::SocketUniquePtr main_ptr;
        EXPECT_EQ(melon::rpc::Socket::Address(channel._server_id, &main_ptr), 0);
        melon::rpc::SocketId agent_id = main_ptr->_agent_socket_id.load(std::memory_order_relaxed);

        for (int i = 0; i < 4; i++) {
            melon::rpc::Controller cntl;
            cntl.set_timeout_ms(50);
            cntl.http_request().set_method(melon::rpc::HTTP_METHOD_POST);
            cntl.http_request().uri() = "/EchoService/Echo?sleep_ms=300";
            channel.CallMethod(nullptr, &cntl, &req, &res, nullptr);
            ASSERT_TRUE(cntl.Failed());

            melon::rpc::SocketUniquePtr ptr;
            melon::rpc::SocketId id = main_ptr->_agent_socket_id.load(std::memory_order_relaxed);
            EXPECT_EQ(id, agent_id);
        }

        {
            // make a successful call again to make sure agent_socket not changing
            melon::rpc::Controller cntl;
            cntl.http_request().set_method(melon::rpc::HTTP_METHOD_POST);
            cntl.http_request().uri() = "/EchoService/Echo";
            channel.CallMethod(nullptr, &cntl, &req, &res, nullptr);
            ASSERT_FALSE(cntl.Failed());
            ASSERT_EQ(EXP_RESPONSE, res.message());
            melon::rpc::SocketUniquePtr ptr;
            melon::rpc::SocketId id = main_ptr->_agent_socket_id.load(std::memory_order_relaxed);
            EXPECT_EQ(id, agent_id);
        }
    }

    TEST_F(HttpTest, http2_header_after_data) {
        melon::rpc::Controller cntl;

        // Prepare request
        melon::cord_buf req_out;
        int h2_stream_id = 0;
        MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);

        // Prepare response to res_out
        melon::cord_buf res_out;
        {
            melon::cord_buf data_buf;
            test::EchoResponse res;
            res.set_message(EXP_RESPONSE);
            {
                melon::cord_buf_as_zero_copy_output_stream wrapper(&data_buf);
                EXPECT_TRUE(res.SerializeToZeroCopyStream(&wrapper));
            }
            melon::rpc::policy::H2Context *ctx =
                    static_cast<melon::rpc::policy::H2Context *>(_h2_client_sock->parsing_context());
            melon::rpc::HPacker &hpacker = ctx->hpacker();
            melon::cord_buf_appender header1_appender;
            melon::rpc::HPackOptions options;
            options.encode_name = false;    /* disable huffman encoding */
            options.encode_value = false;
            {
                melon::rpc::HPacker::Header header(":status", "200");
                hpacker.Encode(&header1_appender, header, options);
            }
            {
                melon::rpc::HPacker::Header header("content-length",
                                                   melon::string_printf("%llu", (unsigned long long) data_buf.size()));
                hpacker.Encode(&header1_appender, header, options);
            }
            {
                melon::rpc::HPacker::Header header(":status", "200");
                hpacker.Encode(&header1_appender, header, options);
            }
            {
                melon::rpc::HPacker::Header header("content-type", "application/proto");
                hpacker.Encode(&header1_appender, header, options);
            }
            {
                melon::rpc::HPacker::Header header("user-defined1", "a");
                hpacker.Encode(&header1_appender, header, options);
            }
            melon::cord_buf header1;
            header1_appender.move_to(header1);

            char headbuf[melon::rpc::policy::FRAME_HEAD_SIZE];
            melon::rpc::policy::SerializeFrameHead(headbuf, header1.size(),
                                                   melon::rpc::policy::H2_FRAME_HEADERS, 0, h2_stream_id);
            // append header1
            res_out.append(headbuf, sizeof(headbuf));
            res_out.append(melon::cord_buf::Movable(header1));

            melon::rpc::policy::SerializeFrameHead(headbuf, data_buf.size(),
                                                   melon::rpc::policy::H2_FRAME_DATA, 0, h2_stream_id);
            // append data
            res_out.append(headbuf, sizeof(headbuf));
            res_out.append(melon::cord_buf::Movable(data_buf));

            melon::cord_buf_appender header2_appender;
            {
                melon::rpc::HPacker::Header header("user-defined1", "overwrite-a");
                hpacker.Encode(&header2_appender, header, options);
            }
            {
                melon::rpc::HPacker::Header header("user-defined2", "b");
                hpacker.Encode(&header2_appender, header, options);
            }
            melon::cord_buf header2;
            header2_appender.move_to(header2);

            melon::rpc::policy::SerializeFrameHead(headbuf, header2.size(),
                                                   melon::rpc::policy::H2_FRAME_HEADERS,
                                                   0x05/* end header and stream */,
                                                   h2_stream_id);
            // append header2
            res_out.append(headbuf, sizeof(headbuf));
            res_out.append(melon::cord_buf::Movable(header2));
        }
        // parse response
        melon::rpc::ParseResult res_pr =
                melon::rpc::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, nullptr);
        ASSERT_TRUE(res_pr.is_ok());
        // process response
        ProcessMessage(melon::rpc::policy::ProcessHttpResponse, res_pr.message(), false);
        ASSERT_FALSE(cntl.Failed());

        melon::rpc::HttpHeader &res_header = cntl.http_response();
        ASSERT_EQ(res_header.content_type(), "application/proto");
        // Check overlapped header is overwritten by the latter.
        const std::string *user_defined1 = res_header.GetHeader("user-defined1");
        ASSERT_EQ(*user_defined1, "overwrite-a");
        const std::string *user_defined2 = res_header.GetHeader("user-defined2");
        ASSERT_EQ(*user_defined2, "b");
    }

    TEST_F(HttpTest, http2_goaway_sanity) {
        melon::rpc::Controller cntl;
        // Prepare request
        melon::cord_buf req_out;
        int h2_stream_id = 0;
        MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);
        // Prepare response
        melon::cord_buf res_out;
        MakeH2EchoResponseBuf(&res_out, h2_stream_id);
        // append goaway
        char goawaybuf[9 /*FRAME_HEAD_SIZE*/ + 8];
        melon::rpc::policy::SerializeFrameHead(goawaybuf, 8, melon::rpc::policy::H2_FRAME_GOAWAY, 0, 0);
        SaveUint32(goawaybuf + 9, 0x7fffd8ef /*last stream id*/);
        SaveUint32(goawaybuf + 13, melon::rpc::H2_NO_ERROR);
        res_out.append(goawaybuf, sizeof(goawaybuf));
        // parse response
        melon::rpc::ParseResult res_pr =
                melon::rpc::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, nullptr);
        ASSERT_TRUE(res_pr.is_ok());
        // process response
        ProcessMessage(melon::rpc::policy::ProcessHttpResponse, res_pr.message(), false);
        ASSERT_TRUE(!cntl.Failed());

        // parse GOAWAY
        res_pr = melon::rpc::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, nullptr);
        ASSERT_EQ(res_pr.error(), melon::rpc::PARSE_ERROR_NOT_ENOUGH_DATA);

        // Since GOAWAY has been received, the next request should fail
        melon::rpc::policy::H2UnsentRequest *h2_req = melon::rpc::policy::H2UnsentRequest::New(&cntl);
        cntl._current_call.stream_user_data = h2_req;
        melon::rpc::SocketMessage *socket_message = nullptr;
        melon::rpc::policy::PackH2Request(nullptr, &socket_message, cntl.call_id().value,
                                          nullptr, &cntl, melon::cord_buf(), nullptr);
        melon::cord_buf dummy;
        melon::result_status st = socket_message->AppendAndDestroySelf(&dummy, _h2_client_sock.get());
        ASSERT_EQ(st.error_code(), melon::rpc::ELOGOFF);
        ASSERT_TRUE(melon::ends_with(st.error_data(), "the connection just issued GOAWAY"));
    }

    class AfterRecevingGoAway : public ::google::protobuf::Closure {
    public:
        void Run() {
            ASSERT_EQ(melon::rpc::EHTTP, cntl.ErrorCode());
            delete this;
        }

        melon::rpc::Controller cntl;
    };

    TEST_F(HttpTest, http2_handle_goaway_streams) {
        const melon::base::end_point ep(melon::base::IP_ANY, 5961);
        melon::base::fd_guard listenfd(melon::base::tcp_listen(ep));
        ASSERT_GT(listenfd, 0);

        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions options;
        options.protocol = melon::rpc::PROTOCOL_H2;
        ASSERT_EQ(0, channel.Init(ep, &options));

        int req_size = 10;
        std::vector<melon::rpc::CallId> ids(req_size);
        for (int i = 0; i < req_size; i++) {
            AfterRecevingGoAway *done = new AfterRecevingGoAway;
            melon::rpc::Controller &cntl = done->cntl;
            ids.push_back(cntl.call_id());
            cntl.set_timeout_ms(-1);
            cntl.http_request().uri() = "/it-doesnt-matter";
            channel.CallMethod(nullptr, &cntl, nullptr, nullptr, done);
        }

        int servfd = accept(listenfd, nullptr, nullptr);
        ASSERT_GT(servfd, 0);
        // Sleep for a while to make sure that server has received all data.
        melon::fiber_sleep_for(2000);
        char goawaybuf[melon::rpc::policy::FRAME_HEAD_SIZE + 8];
        SerializeFrameHead(goawaybuf, 8, melon::rpc::policy::H2_FRAME_GOAWAY, 0, 0);
        SaveUint32(goawaybuf + melon::rpc::policy::FRAME_HEAD_SIZE, 0);
        SaveUint32(goawaybuf + melon::rpc::policy::FRAME_HEAD_SIZE + 4, 0);
        ASSERT_EQ((ssize_t) melon::rpc::policy::FRAME_HEAD_SIZE + 8,
                  ::write(servfd, goawaybuf, melon::rpc::policy::FRAME_HEAD_SIZE + 8));

        // After receving GOAWAY, the callbacks in client should be run correctly.
        for (int i = 0; i < req_size; i++) {
            melon::rpc::Join(ids[i]);
        }
    }

    TEST_F(HttpTest, dump_http_request) {
        // save origin value of gflag
        auto rpc_dump_dir = melon::rpc::FLAGS_rpc_dump_dir;
        auto rpc_dump_max_requests_in_one_file = melon::rpc::FLAGS_rpc_dump_max_requests_in_one_file;

        // set gflag and global variable in order to be sure to dump request
        melon::rpc::FLAGS_rpc_dump = true;
        melon::rpc::FLAGS_rpc_dump_dir = "dump_http_request";
        melon::rpc::FLAGS_rpc_dump_max_requests_in_one_file = 1;
        melon::rpc::g_rpc_dump_sl.ever_grabbed = true;
        melon::rpc::g_rpc_dump_sl.sampling_range = melon::COLLECTOR_SAMPLING_BASE;

        // init channel
        const int port = 8923;
        melon::rpc::Server server;
        EXPECT_EQ(0, server.AddService(&_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));

        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions options;
        options.protocol = "http";
        ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));

        // send request and dump it to file
        {
            test::EchoRequest req;
            req.set_message(EXP_REQUEST);
            std::string req_json;
            ASSERT_TRUE(json2pb::ProtoMessageToJson(req, &req_json));

            melon::rpc::Controller cntl;
            cntl.http_request().uri() = "/EchoService/Echo";
            cntl.http_request().set_content_type("application/json");
            cntl.http_request().set_method(melon::rpc::HTTP_METHOD_POST);
            cntl.request_attachment() = req_json;
            channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
            ASSERT_FALSE(cntl.Failed());

            // sleep 1s, because rpc_dump doesn't run immediately
            sleep(1);
        }

        // replay request from dump file
        {
            melon::rpc::SampleIterator it(melon::rpc::FLAGS_rpc_dump_dir);
            melon::rpc::SampledRequest* sample = it.Next();
            ASSERT_NE(nullptr, sample);

            std::unique_ptr<melon::rpc::SampledRequest> sample_guard(sample);

            // the logic of next code is same as that in rpc_replay.cpp
            ASSERT_EQ(sample->meta.protocol_type(), melon::rpc::PROTOCOL_HTTP);
            melon::rpc::Controller cntl;
            cntl.reset_sampled_request(sample_guard.release());
            melon::rpc::HttpMessage http_message;
            http_message.ParseFromCordBuf(sample->request);
            cntl.http_request().Swap(http_message.header());
            // clear origin Host in header
            cntl.http_request().RemoveHeader("Host");
            cntl.http_request().uri().set_host("");
            cntl.request_attachment() = http_message.body().movable();

            channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
            ASSERT_FALSE(cntl.Failed());
            ASSERT_EQ("application/json", cntl.http_response().content_type());

            std::string res_json = cntl.response_attachment().to_string();
            test::EchoResponse res;
            json2pb::Json2PbOptions options;
            ASSERT_TRUE(json2pb::JsonToProtoMessage(res_json, &res, options));
            ASSERT_EQ(EXP_RESPONSE, res.message());
        }

        // delete dump directory
        melon::remove_all(melon::rpc::FLAGS_rpc_dump_dir);

        // restore gflag and global variable
        melon::rpc::FLAGS_rpc_dump = false;
        melon::rpc::FLAGS_rpc_dump_dir = rpc_dump_dir;
        melon::rpc::FLAGS_rpc_dump_max_requests_in_one_file = rpc_dump_max_requests_in_one_file;
        melon::rpc::g_rpc_dump_sl.ever_grabbed = false;
        melon::rpc::g_rpc_dump_sl.sampling_range = 0;
    }

    TEST_F(HttpTest, spring_protobuf_content_type) {
        const int port = 8923;
        melon::rpc::Server server;
        EXPECT_EQ(0, server.AddService(&_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));

        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions options;
        options.protocol = "http";
        ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));

        melon::rpc::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(EXP_REQUEST);
        cntl.http_request().set_method(melon::rpc::HTTP_METHOD_POST);
        cntl.http_request().uri() = "/EchoService/Echo";
        cntl.http_request().set_content_type("application/x-protobuf");
        cntl.request_attachment().append(req.SerializeAsString());
        channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_EQ("application/x-protobuf", cntl.http_response().content_type());
        ASSERT_TRUE(res.ParseFromString(cntl.response_attachment().to_string()));
        ASSERT_EQ(EXP_RESPONSE, res.message());

        melon::rpc::Controller cntl2;
        test::EchoService_Stub stub(&channel);
        req.set_message(EXP_REQUEST);
        res.Clear();
        cntl2.http_request().set_content_type("application/x-protobuf");
        stub.Echo(&cntl2, &req, &res, nullptr);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_EQ(EXP_RESPONSE, res.message());
        ASSERT_EQ("application/x-protobuf", cntl.http_response().content_type());
    }

    TEST_F(HttpTest, spring_protobuf_text_content_type) {
        const int port = 8923;
        melon::rpc::Server server;
        EXPECT_EQ(0, server.AddService(&_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));

        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions options;
        options.protocol = "http";
        ASSERT_EQ(0, channel.Init(melon::base::end_point(melon::base::my_ip(), port), &options));

        melon::rpc::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(EXP_REQUEST);
        cntl.http_request().set_method(melon::rpc::HTTP_METHOD_POST);
        cntl.http_request().uri() = "/EchoService/Echo";
        cntl.http_request().set_content_type("application/proto-text");
        cntl.request_attachment().append(req.Utf8DebugString());
        channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_EQ("application/proto-text", cntl.http_response().content_type());
        ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
                cntl.response_attachment().to_string(), &res));
        ASSERT_EQ(EXP_RESPONSE, res.message());
    }

} //namespace
