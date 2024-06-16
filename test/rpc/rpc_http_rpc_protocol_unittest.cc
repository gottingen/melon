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




// Date: Sun Jul 13 15:04:18 CST 2014

#include <cstddef>
#include <google/protobuf/stubs/logging.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <melon/base/build_config.h> // OS_MACOSX
#if defined(OS_MACOSX)
#include <sys/event.h>
#endif
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <google/protobuf/text_format.h>
#include <unistd.h>
#include <melon/utility/strings/string_number_conversions.h>
#include <melon/rpc/policy/http_rpc_protocol.h>
#include <turbo/strings/escaping.h>
#include <melon/rpc/http/http_method.h>
#include <melon/base/iobuf.h>
#include <turbo/log/logging.h>
#include <melon/utility/files/scoped_file.h>
#include <melon/base/fd_guard.h>
#include <melon/utility/file_util.h>
#include <melon/rpc/socket.h>
#include <melon/rpc/acceptor.h>
#include <melon/rpc/server.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/policy/most_common_message.h>
#include "echo.pb.h"
#include <melon/rpc/policy/http_rpc_protocol.h>
#include <melon/rpc/policy/http2_rpc_protocol.h>
#include "melon/json2pb/pb_to_json.h"
#include "melon/json2pb/json_to_pb.h"
#include <melon/rpc/details/method_status.h>
#include <melon/rpc/dump/rpc_dump.h>
#include <melon/fiber/unstable.h>
#include <turbo/strings/match.h>

namespace melon {
DECLARE_bool(rpc_dump);
DECLARE_string(rpc_dump_dir);
DECLARE_int32(rpc_dump_max_requests_in_one_file);
extern melon::var::CollectorSpeedLimit g_rpc_dump_sl;
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    if (google::SetCommandLineOption("socket_max_unwritten_bytes", "2000000").empty()) {
        std::cerr << "Fail to set -socket_max_unwritten_bytes" << std::endl;
        return -1;
    }
    return RUN_ALL_TESTS();
}

namespace {

static const std::string EXP_REQUEST = "hello";
static const std::string EXP_RESPONSE = "world";
static const std::string EXP_RESPONSE_CONTENT_LENGTH = "1024";
static const std::string EXP_RESPONSE_TRANSFER_ENCODING = "chunked";

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
public:
    void Echo(::google::protobuf::RpcController* cntl_base,
              const ::test::EchoRequest* req,
              ::test::EchoResponse* res,
              ::google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);
        melon::Controller* cntl =
            static_cast<melon::Controller*>(cntl_base);
        const std::string* sleep_ms_str =
            cntl->http_request().uri().GetQuery("sleep_ms");
        if (sleep_ms_str) {
            fiber_usleep(strtol(sleep_ms_str->data(), NULL, 10) * 1000);
        }
        res->set_message(EXP_RESPONSE);
    }
};

class HttpTest : public ::testing::Test{
protected:
    HttpTest() {
        EXPECT_EQ(0, _server.AddBuiltinServices());
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

        melon::SocketOptions h2_client_options;
        h2_client_options.user = melon::get_client_side_messenger();
        h2_client_options.fd = _pipe_fds[1];
        EXPECT_EQ(0, melon::Socket::Create(h2_client_options, &id));
        EXPECT_EQ(0, melon::Socket::Address(id, &_h2_client_sock));
    };

    virtual ~HttpTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {};

    void VerifyMessage(melon::InputMessageBase* msg, bool expect) {
        if (msg->_socket == NULL) {
            _socket->ReAddress(&msg->_socket);
        }
        msg->_arg = &_server;
        EXPECT_EQ(expect, melon::policy::VerifyHttpRequest(msg));
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

    melon::policy::HttpContext* MakePostRequestMessage(const std::string& path) {
        melon::policy::HttpContext* msg = new melon::policy::HttpContext(false);
        msg->header().uri().set_path(path);
        msg->header().set_content_type("application/json");
        msg->header().set_method(melon::HTTP_METHOD_POST);

        test::EchoRequest req;
        req.set_message(EXP_REQUEST);
        mutil::IOBufAsZeroCopyOutputStream req_stream(&msg->body());
        EXPECT_TRUE(json2pb::ProtoMessageToJson(req, &req_stream, NULL));
        return msg;
    }

    melon::policy::HttpContext* MakePostProtoTextRequestMessage(
        const std::string& path) {
        melon::policy::HttpContext* msg = new melon::policy::HttpContext(false);
        msg->header().uri().set_path(path);
        msg->header().set_content_type("application/proto-text");
        msg->header().set_method(melon::HTTP_METHOD_POST);

        test::EchoRequest req;
        req.set_message(EXP_REQUEST);
        mutil::IOBufAsZeroCopyOutputStream req_stream(&msg->body());
        EXPECT_TRUE(google::protobuf::TextFormat::Print(req, &req_stream));
        return msg;
    }

    melon::policy::HttpContext* MakeGetRequestMessage(const std::string& path) {
        melon::policy::HttpContext* msg = new melon::policy::HttpContext(false);
        msg->header().uri().set_path(path);
        msg->header().set_method(melon::HTTP_METHOD_GET);
        return msg;
    }


    melon::policy::HttpContext* MakeResponseMessage(int code) {
        melon::policy::HttpContext* msg = new melon::policy::HttpContext(false);
        msg->header().set_status_code(code);
        msg->header().set_content_type("application/json");
        
        test::EchoResponse res;
        res.set_message(EXP_RESPONSE);
        mutil::IOBufAsZeroCopyOutputStream res_stream(&msg->body());
        EXPECT_TRUE(json2pb::ProtoMessageToJson(res, &res_stream, NULL));
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
        melon::ParseResult pr =
                melon::policy::ParseHttpMessage(&buf, _socket.get(), false, NULL);
        EXPECT_EQ(melon::PARSE_OK, pr.error());
        melon::policy::HttpContext* msg =
            static_cast<melon::policy::HttpContext*>(pr.message());

        EXPECT_EQ(expect_code, msg->header().status_code());
        msg->Destroy();
    }

    void MakeH2EchoRequestBuf(mutil::IOBuf* out, melon::Controller* cntl, int* h2_stream_id) {
        mutil::IOBuf request_buf;
        test::EchoRequest req;
        req.set_message(EXP_REQUEST);
        cntl->http_request().set_method(melon::HTTP_METHOD_POST);
        melon::policy::SerializeHttpRequest(&request_buf, cntl, &req);
        ASSERT_FALSE(cntl->Failed());
        melon::policy::H2UnsentRequest* h2_req = melon::policy::H2UnsentRequest::New(cntl);
        cntl->_current_call.stream_user_data = h2_req;
        melon::SocketMessage* socket_message = NULL;
        melon::policy::PackH2Request(NULL, &socket_message, cntl->call_id().value,
                                    NULL, cntl, request_buf, NULL);
        mutil::Status st = socket_message->AppendAndDestroySelf(out, _h2_client_sock.get());
        ASSERT_TRUE(st.ok());
        *h2_stream_id = h2_req->_stream_id;
    }

    void MakeH2EchoResponseBuf(mutil::IOBuf* out, int h2_stream_id) {
        melon::Controller cntl;
        test::EchoResponse res;
        res.set_message(EXP_RESPONSE);
        cntl.http_request().set_content_type("application/proto");
        {
            mutil::IOBufAsZeroCopyOutputStream wrapper(&cntl.response_attachment());
            EXPECT_TRUE(res.SerializeToZeroCopyStream(&wrapper));
        }
        melon::policy::H2UnsentResponse* h2_res = melon::policy::H2UnsentResponse::New(&cntl, h2_stream_id, false /*is grpc*/);
        mutil::Status st = h2_res->AppendAndDestroySelf(out, _h2_client_sock.get());
        ASSERT_TRUE(st.ok());
    }

    int _pipe_fds[2];
    melon::SocketUniquePtr _socket;
    melon::SocketUniquePtr _h2_client_sock;
    melon::Server _server;

    MyEchoService _svc;
    MyAuthenticator _auth;
};

TEST_F(HttpTest, indenting_ostream) {
    std::ostringstream os1;
    melon::IndentingOStream is1(os1, 2);
    melon::IndentingOStream is2(is1, 2);
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
    mutil::EndPoint EXP_ENDPOINT;
    {
        std::string url = "https://" + EXP_HOSTNAME;
        EXPECT_TRUE(melon::policy::ParseHttpServerAddress(&EXP_ENDPOINT, url.c_str()));
    }
    {
        mutil::EndPoint ep;
        std::string url = "http://" +
                          std::string(endpoint2str(EXP_ENDPOINT).c_str());
        EXPECT_TRUE(melon::policy::ParseHttpServerAddress(&ep, url.c_str()));
        EXPECT_EQ(EXP_ENDPOINT, ep);
    }
    {
        mutil::EndPoint ep;
        std::string url = "https://" +
            std::string(mutil::ip2str(EXP_ENDPOINT.ip).c_str());
        EXPECT_TRUE(melon::policy::ParseHttpServerAddress(&ep, url.c_str()));
        EXPECT_EQ(EXP_ENDPOINT.ip, ep.ip);
        EXPECT_EQ(443, ep.port);
    }
    {
        mutil::EndPoint ep;
        EXPECT_FALSE(melon::policy::ParseHttpServerAddress(&ep, "invalid_url"));
    }
    {
        mutil::EndPoint ep;
        EXPECT_FALSE(melon::policy::ParseHttpServerAddress(
            &ep, "https://no.such.machine:9090"));
    }
}

TEST_F(HttpTest, verify_request) {
    {
        melon::policy::HttpContext* msg =
                MakePostRequestMessage("/EchoService/Echo");
        VerifyMessage(msg, false);
        msg->Destroy();
    }
    {
        melon::policy::HttpContext* msg = MakeGetRequestMessage("/status");
        VerifyMessage(msg, true);
        msg->Destroy();
    }
    {
        melon::policy::HttpContext* msg =
                MakePostRequestMessage("/EchoService/Echo");
        _socket->SetFailed();
        VerifyMessage(msg, false);
        msg->Destroy();
    }
    {
        melon::policy::HttpContext* msg =
                MakePostProtoTextRequestMessage("/EchoService/Echo");
        VerifyMessage(msg, false);
        msg->Destroy();
    }
}

TEST_F(HttpTest, process_request_failed_socket) {
    melon::policy::HttpContext* msg = MakePostRequestMessage("/EchoService/Echo");
    _socket->SetFailed();
    ProcessMessage(melon::policy::ProcessHttpRequest, msg, false);
    ASSERT_EQ(0ll, _server._nerror_var.get_value());
    CheckResponseCode(true, 0);
}

TEST_F(HttpTest, reject_get_to_pb_services_with_required_fields) {
    melon::policy::HttpContext* msg = MakeGetRequestMessage("/EchoService/Echo");
    _server._status = melon::Server::RUNNING;
    ProcessMessage(melon::policy::ProcessHttpRequest, msg, false);
    ASSERT_EQ(0ll, _server._nerror_var.get_value());
    const melon::Server::MethodProperty* mp =
        _server.FindMethodPropertyByFullName("test.EchoService.Echo");
    ASSERT_TRUE(mp);
    ASSERT_TRUE(mp->status);
    ASSERT_EQ(1ll, mp->status->_nerror_var.get_value());
    CheckResponseCode(false, melon::HTTP_STATUS_BAD_REQUEST);
}

TEST_F(HttpTest, process_request_logoff) {
    melon::policy::HttpContext* msg = MakePostRequestMessage("/EchoService/Echo");
    _server._status = melon::Server::READY;
    ProcessMessage(melon::policy::ProcessHttpRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_var.get_value());
    CheckResponseCode(false, melon::HTTP_STATUS_SERVICE_UNAVAILABLE);
}

TEST_F(HttpTest, process_request_wrong_method) {
    melon::policy::HttpContext* msg = MakePostRequestMessage("/NO_SUCH_METHOD");
    ProcessMessage(melon::policy::ProcessHttpRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_var.get_value());
    CheckResponseCode(false, melon::HTTP_STATUS_NOT_FOUND);
}

TEST_F(HttpTest, process_response_after_eof) {
    test::EchoResponse res;
    melon::Controller cntl;
    cntl._response = &res;
    melon::policy::HttpContext* msg =
            MakeResponseMessage(melon::HTTP_STATUS_OK);
    _socket->set_correlation_id(cntl.call_id().value);
    ProcessMessage(melon::policy::ProcessHttpResponse, msg, true);
    ASSERT_EQ(EXP_RESPONSE, res.message());
    ASSERT_TRUE(_socket->Failed());
}

TEST_F(HttpTest, process_response_error_code) {
    {
        melon::Controller cntl;
        _socket->set_correlation_id(cntl.call_id().value);
        melon::policy::HttpContext* msg =
                MakeResponseMessage(melon::HTTP_STATUS_CONTINUE);
        ProcessMessage(melon::policy::ProcessHttpResponse, msg, false);
        ASSERT_EQ(melon::EHTTP, cntl.ErrorCode());
        ASSERT_EQ(melon::HTTP_STATUS_CONTINUE, cntl.http_response().status_code());
    }
    {
        melon::Controller cntl;
        _socket->set_correlation_id(cntl.call_id().value);
        melon::policy::HttpContext* msg =
                MakeResponseMessage(melon::HTTP_STATUS_TEMPORARY_REDIRECT);
        ProcessMessage(melon::policy::ProcessHttpResponse, msg, false);
        ASSERT_EQ(melon::EHTTP, cntl.ErrorCode());
        ASSERT_EQ(melon::HTTP_STATUS_TEMPORARY_REDIRECT,
                  cntl.http_response().status_code());
    }
    {
        melon::Controller cntl;
        _socket->set_correlation_id(cntl.call_id().value);
        melon::policy::HttpContext* msg =
                MakeResponseMessage(melon::HTTP_STATUS_BAD_REQUEST);
        ProcessMessage(melon::policy::ProcessHttpResponse, msg, false);
        ASSERT_EQ(melon::EHTTP, cntl.ErrorCode());
        ASSERT_EQ(melon::HTTP_STATUS_BAD_REQUEST,
                  cntl.http_response().status_code());
    }
    {
        melon::Controller cntl;
        _socket->set_correlation_id(cntl.call_id().value);
        melon::policy::HttpContext* msg = MakeResponseMessage(12345);
        ProcessMessage(melon::policy::ProcessHttpResponse, msg, false);
        ASSERT_EQ(melon::EHTTP, cntl.ErrorCode());
        ASSERT_EQ(12345, cntl.http_response().status_code());
    }
}

TEST_F(HttpTest, complete_flow) {
    mutil::IOBuf request_buf;
    mutil::IOBuf total_buf;
    melon::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    cntl._response = &res;
    cntl._connection_type = melon::CONNECTION_TYPE_SHORT;
    cntl._method = test::EchoService::descriptor()->method(0);
    ASSERT_EQ(0, melon::Socket::Address(_socket->id(), &cntl._current_call.sending_sock));

    // Send request
    req.set_message(EXP_REQUEST);
    melon::policy::SerializeHttpRequest(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    melon::policy::PackHttpRequest(
        &total_buf, NULL, cntl.call_id().value,
        cntl._method, &cntl, request_buf, &_auth);
    ASSERT_FALSE(cntl.Failed());

    // Verify and handle request
    melon::ParseResult req_pr =
            melon::policy::ParseHttpMessage(&total_buf, _socket.get(), false, NULL);
    ASSERT_EQ(melon::PARSE_OK, req_pr.error());
    melon::InputMessageBase* req_msg = req_pr.message();
    VerifyMessage(req_msg, true);
    ProcessMessage(melon::policy::ProcessHttpRequest, req_msg, false);

    // Read response from pipe
    mutil::IOPortal response_buf;
    response_buf.append_from_file_descriptor(_pipe_fds[0], 1024);
    melon::ParseResult res_pr =
            melon::policy::ParseHttpMessage(&response_buf, _socket.get(), false, NULL);
    ASSERT_EQ(melon::PARSE_OK, res_pr.error());
    melon::InputMessageBase* res_msg = res_pr.message();
    ProcessMessage(melon::policy::ProcessHttpResponse, res_msg, false);

    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(EXP_RESPONSE, res.message());
}

TEST_F(HttpTest, chunked_uploading) {
    const int port = 8923;
    melon::Server server;
    EXPECT_EQ(0, server.AddService(&_svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, NULL));

    // Send request via curl using chunked encoding
    const std::string req = "{\"message\":\"hello\"}";
    const std::string res_fname = "curl.out";
    std::string cmd = turbo::str_format("curl -X POST -d '%s' -H 'Transfer-Encoding:chunked' "
                        "-H 'Content-Type:application/json' -o %s "
                        "http://localhost:%d/EchoService/Echo",
                        req.c_str(), res_fname.c_str(), port);
    ASSERT_EQ(0, system(cmd.c_str()));

    // Check response
    const std::string exp_res = "{\"message\":\"world\"}";
    mutil::ScopedFILE fp(res_fname.c_str(), "r");
    char buf[128];
    ASSERT_TRUE(fgets(buf, sizeof(buf), fp));
    EXPECT_EQ(exp_res, std::string(buf));
}

enum DonePlace {
    DONE_BEFORE_CREATE_PA = 0,
    DONE_AFTER_CREATE_PA_BEFORE_DESTROY_PA,
    DONE_AFTER_DESTROY_PA,
};
// For writing into PA.
const char PA_DATA[] = "abcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()_=-+";
const size_t PA_DATA_LEN = sizeof(PA_DATA) - 1/*not count the ending zero*/;

static void CopyPAPrefixedWithSeqNo(char* buf, uint64_t seq_no) {
    memcpy(buf, PA_DATA, PA_DATA_LEN);
    *(uint64_t*)buf = seq_no;
}

class DownloadServiceImpl : public ::test::DownloadService {
public:
    DownloadServiceImpl(DonePlace done_place = DONE_BEFORE_CREATE_PA,
                        size_t num_repeat = 1)
        : _done_place(done_place)
        , _nrep(num_repeat)
        , _nwritten(0)
        , _ever_full(false)
        , _last_errno(0) {}
    
    void Download(::google::protobuf::RpcController* cntl_base,
                  const ::test::HttpRequest*,
                  ::test::HttpResponse*,
                  ::google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);
        melon::Controller* cntl =
            static_cast<melon::Controller*>(cntl_base);
        cntl->http_response().set_content_type("text/plain");
        melon::StopStyle stop_style = (_nrep == std::numeric_limits<size_t>::max()
                ? melon::FORCE_STOP : melon::WAIT_FOR_STOP);
        mutil::intrusive_ptr<melon::ProgressiveAttachment> pa
            = cntl->CreateProgressiveAttachment(stop_style);
        if (pa == NULL) {
            cntl->SetFailed("The socket was just failed");
            return;
        }
        if (_done_place == DONE_BEFORE_CREATE_PA) {
            done_guard.reset(NULL);
        }
        ASSERT_GT(PA_DATA_LEN, 8u);  // long enough to hold a 64-bit decimal.
        char buf[PA_DATA_LEN];
        for (size_t c = 0; c < _nrep;) {
            CopyPAPrefixedWithSeqNo(buf, c);
            if (pa->Write(buf, sizeof(buf)) != 0) {
                if (errno == melon::EOVERCROWDED) {
                    LOG_EVERY_N_SEC(INFO, 1) << "full pa=" << pa.get();
                    _ever_full = true;
                    fiber_usleep(10000);
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
            done_guard.reset(NULL);
        }
        LOG(INFO) << "Destroy pa="  << pa.get();
        pa.reset(NULL);
        if (_done_place == DONE_AFTER_DESTROY_PA) {
            done_guard.reset(NULL);
        }
    }

    void DownloadFailed(::google::protobuf::RpcController* cntl_base,
                        const ::test::HttpRequest*,
                        ::test::HttpResponse*,
                        ::google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);
        melon::Controller* cntl =
            static_cast<melon::Controller*>(cntl_base);
        cntl->http_response().set_content_type("text/plain");
        melon::StopStyle stop_style = (_nrep == std::numeric_limits<size_t>::max()
                ? melon::FORCE_STOP : melon::WAIT_FOR_STOP);
        mutil::intrusive_ptr<melon::ProgressiveAttachment> pa
            = cntl->CreateProgressiveAttachment(stop_style);
        if (pa == NULL) {
            cntl->SetFailed("The socket was just failed");
            return;
        }
        char buf[PA_DATA_LEN];
        while (true) {
            if (pa->Write(buf, sizeof(buf)) != 0) {
                if (errno == melon::EOVERCROWDED) {
                    LOG_EVERY_N_SEC(INFO, 1) << "full pa=" << pa.get();
                    fiber_usleep(10000);
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
        done_guard.reset(NULL);
        
        // Return value of Write after controller has failed should
        // be less than zero.
        CHECK_LT(pa->Write(buf, sizeof(buf)), 0);
        CHECK_EQ(errno, ECANCELED);
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
    melon::Server server;
    DownloadServiceImpl svc;
    EXPECT_EQ(0, server.AddService(&svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, NULL));

    for (int i = 0; i < 3; ++i) {
        svc.set_done_place((DonePlace)i);
        melon::Channel channel;
        melon::ChannelOptions options;
        options.protocol = melon::PROTOCOL_HTTP;
        ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));
        melon::Controller cntl;
        cntl.http_request().uri() = "/DownloadService/Download";
        channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();

        std::string expected(PA_DATA_LEN, 0);
        CopyPAPrefixedWithSeqNo(&expected[0], 0);
        ASSERT_EQ(expected, cntl.response_attachment());
    }
}

TEST_F(HttpTest, read_failed_chunked_response) {
    const int port = 8923;
    melon::Server server;
    DownloadServiceImpl svc;
    EXPECT_EQ(0, server.AddService(&svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, NULL));

    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_HTTP;
    ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));

    melon::Controller cntl;
    cntl.http_request().uri() = "/DownloadService/DownloadFailed";
    cntl.response_will_be_read_progressively();
    channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
    EXPECT_TRUE(cntl.response_attachment().empty());
    ASSERT_TRUE(cntl.Failed());
    ASSERT_NE(cntl.ErrorText().find("HTTP/1.1 500 Internal Server Error"),
              std::string::npos) << cntl.ErrorText();
    ASSERT_NE(cntl.ErrorText().find("Intentionally set controller failed"),
              std::string::npos) << cntl.ErrorText();
    ASSERT_EQ(0, svc.last_errno());
}

class ReadBody : public melon::ProgressiveReader,
                 public melon::SharedObject {
public:
    ReadBody()
        : _nread(0)
        , _ncount(0)
        , _destroyed(false) {
        mutil::intrusive_ptr<ReadBody>(this).detach(); // ref
    }
                
    mutil::Status OnReadOnePart(const void* data, size_t length) {
        _nread += length;
        while (length > 0) {
            size_t nappend = std::min(_buf.size() + length, PA_DATA_LEN) - _buf.size();
            _buf.append((const char*)data, nappend);
            data = (const char*)data + nappend;
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
        return mutil::Status::OK();
    }
    void OnEndOfMessage(const mutil::Status& st) {
        mutil::intrusive_ptr<ReadBody>(this, false); // deref
        ASSERT_LT(_buf.size(), PA_DATA_LEN);
        ASSERT_EQ(0, memcmp(_buf.data(), PA_DATA, _buf.size()));
        _destroyed = true;
        _destroying_st = st;
        LOG(INFO) << "Destroy ReadBody=" << this << ", " << st;
    }
    bool destroyed() const { return _destroyed; }
    const mutil::Status& destroying_status() const { return _destroying_st; }
    size_t read_bytes() const { return _nread; }
private:
    std::string _buf;
    size_t _nread;
    size_t _ncount;
    bool _destroyed;
    mutil::Status _destroying_st;
};

static const int GENERAL_DELAY_US = 300000; // 0.3s

TEST_F(HttpTest, read_long_body_progressively) {
    mutil::intrusive_ptr<ReadBody> reader;
    {
        const int port = 8923;
        melon::Server server;
        DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                std::numeric_limits<size_t>::max());
        EXPECT_EQ(0, server.AddService(&svc, melon::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, NULL));
        {
            melon::Channel channel;
            melon::ChannelOptions options;
            options.protocol = melon::PROTOCOL_HTTP;
            ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));
            {
                melon::Controller cntl;
                cntl.response_will_be_read_progressively();
                cntl.http_request().uri() = "/DownloadService/Download";
                channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
                ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
                ASSERT_TRUE(cntl.response_attachment().empty());
                reader.reset(new ReadBody);
                cntl.ReadProgressiveAttachmentBy(reader.get());
                size_t last_read = 0;
                for (size_t i = 0; i < 3; ++i) {
                    sleep(1);
                    size_t current_read = reader->read_bytes();
                    LOG(INFO) << "read=" << current_read - last_read
                              << " total=" << current_read;
                    last_read = current_read;
                }
                // Read something in past N seconds.
                ASSERT_GT(last_read, (size_t)100000);
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
    mutil::intrusive_ptr<ReadBody> reader;
    const int port = 8923;
    melon::Server server;
    const int NREP = 10000;
    DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA, NREP);
    EXPECT_EQ(0, server.AddService(&svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, NULL));
    {
        melon::Channel channel;
        melon::ChannelOptions options;
        options.protocol = melon::PROTOCOL_HTTP;
        ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));
        {
            melon::Controller cntl;
            cntl.response_will_be_read_progressively();
            cntl.http_request().uri() = "/DownloadService/Download";
            channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
            ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
            ASSERT_TRUE(cntl.response_attachment().empty());
            reader.reset(new ReadBody);
            cntl.ReadProgressiveAttachmentBy(reader.get());
            size_t last_read = 0;
            for (size_t i = 0; i < 3; ++i) {
                sleep(1);
                size_t current_read = reader->read_bytes();
                LOG(INFO) << "read=" << current_read - last_read
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
    mutil::intrusive_ptr<ReadBody> reader;
    {
        const int port = 8923;
        melon::Server server;
        DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                std::numeric_limits<size_t>::max());
        EXPECT_EQ(0, server.AddService(&svc, melon::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, NULL));
        {
            melon::Channel channel;
            melon::ChannelOptions options;
            options.protocol = melon::PROTOCOL_HTTP;
            ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));
            {
                melon::Controller cntl;
                cntl.response_will_be_read_progressively();
                cntl.http_request().uri() = "/DownloadService/Download";
                channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
                ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
                ASSERT_TRUE(cntl.response_attachment().empty());
                reader.reset(new ReadBody);
                cntl.ReadProgressiveAttachmentBy(reader.get());
            }
            size_t last_read = 0;
            for (size_t i = 0; i < 3; ++i) {
                sleep(1);
                size_t current_read = reader->read_bytes();
                LOG(INFO) << "read=" << current_read - last_read
                          << " total=" << current_read;
                last_read = current_read;
            }
            // Read something in past N seconds.
            ASSERT_GT(last_read, (size_t)100000);
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
    mutil::intrusive_ptr<ReadBody> reader;
    {
        const int port = 8923;
        melon::Server server;
        DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                std::numeric_limits<size_t>::max());
        EXPECT_EQ(0, server.AddService(&svc, melon::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, NULL));
        {
            melon::Channel channel;
            melon::ChannelOptions options;
            options.protocol = melon::PROTOCOL_HTTP;
            ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));
            {
                melon::Controller cntl;
                cntl.response_will_be_read_progressively();
                cntl.http_request().uri() = "/DownloadService/Download";
                channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
                ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
                ASSERT_TRUE(cntl.response_attachment().empty());
                LOG(INFO) << "Sleep 3 seconds to make PA at server-side full";
                sleep(3);
                EXPECT_TRUE(svc.ever_full());
                ASSERT_EQ(0, svc.last_errno());
                reader.reset(new ReadBody);
                cntl.ReadProgressiveAttachmentBy(reader.get());
                size_t last_read = 0;
                for (size_t i = 0; i < 3; ++i) {
                    sleep(1);
                    size_t current_read = reader->read_bytes();
                    LOG(INFO) << "read=" << current_read - last_read
                              << " total=" << current_read;
                    last_read = current_read;
                }
                // Read something in past N seconds.
                ASSERT_GT(last_read, (size_t)100000);
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
    melon::Server server;
    DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                            std::numeric_limits<size_t>::max());
    EXPECT_EQ(0, server.AddService(&svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, NULL));
    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_HTTP;
    ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));
    {
        melon::Controller cntl;
        cntl.response_will_be_read_progressively();
        cntl.http_request().uri() = "/DownloadService/Download";
        channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        ASSERT_TRUE(cntl.response_attachment().empty());
    }
    const size_t old_written_bytes = svc.written_bytes();
    LOG(INFO) << "Sleep 3 seconds after destroy of Controller";
    sleep(3);
    const size_t new_written_bytes = svc.written_bytes();
    ASSERT_EQ(0, svc.last_errno());
    LOG(INFO) << "Server still wrote " << new_written_bytes - old_written_bytes;
    // The server side still wrote things.
    ASSERT_GT(new_written_bytes - old_written_bytes, (size_t)100000);
}

class AlwaysFailRead : public melon::ProgressiveReader {
public:
    // @ProgressiveReader
    mutil::Status OnReadOnePart(const void* /*data*/, size_t /*length*/) {
        return mutil::Status(-1, "intended fail at %s:%d", __FILE__, __LINE__);
    }
    void OnEndOfMessage(const mutil::Status& st) {
        LOG(INFO) << "Destroy " << this << ": " << st;
        delete this;
    }
};

TEST_F(HttpTest, failed_on_read_one_part) {
    const int port = 8923;
    melon::Server server;
    DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                            std::numeric_limits<size_t>::max());
    EXPECT_EQ(0, server.AddService(&svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, NULL));
    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_HTTP;
    ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));
    {
        melon::Controller cntl;
        cntl.response_will_be_read_progressively();
        cntl.http_request().uri() = "/DownloadService/Download";
        channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        ASSERT_TRUE(cntl.response_attachment().empty());
        cntl.ReadProgressiveAttachmentBy(new AlwaysFailRead);
    }
    LOG(INFO) << "Sleep 1 second";
    sleep(1);
    ASSERT_NE(0, svc.last_errno());
}

TEST_F(HttpTest, broken_socket_stops_progressive_reading) {
    mutil::intrusive_ptr<ReadBody> reader;
    const int port = 8923;
    melon::Server server;
    DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                            std::numeric_limits<size_t>::max());
    EXPECT_EQ(0, server.AddService(&svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, NULL));
        
    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_HTTP;
    ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));
    {
        melon::Controller cntl;
        cntl.response_will_be_read_progressively();
        cntl.http_request().uri() = "/DownloadService/Download";
        channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        ASSERT_TRUE(cntl.response_attachment().empty());
        reader.reset(new ReadBody);
        cntl.ReadProgressiveAttachmentBy(reader.get());
        size_t last_read = 0;
        for (size_t i = 0; i < 3; ++i) {
            sleep(1);
            size_t current_read = reader->read_bytes();
            LOG(INFO) << "read=" << current_read - last_read
                      << " total=" << current_read;
            last_read = current_read;
        }
        // Read something in past N seconds.
        ASSERT_GT(last_read, (size_t)100000);
    }
    // the socket still holds a ref.
    ASSERT_FALSE(reader->destroyed());
    LOG(INFO) << "Stopping the server";
    server.Stop(0);
    server.Join();
        
    // Wait for error reporting from the socket.
    usleep(GENERAL_DELAY_US);
    ASSERT_TRUE(reader->destroyed());
    ASSERT_EQ(ECONNRESET, reader->destroying_status().error_code());
}

static const std::string TEST_PROGRESSIVE_HEADER = "Progressive";
static const std::string TEST_PROGRESSIVE_HEADER_VAL = "Progressive-val";

class ServerProgressiveReader : public ReadBody {
public:
    ServerProgressiveReader(melon::Controller* cntl, google::protobuf::Closure* done)
        : _cntl(cntl)
        , _done(done) {}

    // @ProgressiveReader
    void OnEndOfMessage(const mutil::Status& st) {
        mutil::intrusive_ptr<ReadBody>(this);
        melon::ClosureGuard done_guard(_done);
        ASSERT_LT(_buf.size(), PA_DATA_LEN);
        ASSERT_EQ(0, memcmp(_buf.data(), PA_DATA, _buf.size()));
        _destroyed = true;
        _destroying_st = st;
        LOG(INFO) << "Destroy ReadBody=" << this << ", " << st;
        _cntl->response_attachment().append("Sucess");
    }
private:
    melon::Controller* _cntl;
    google::protobuf::Closure* _done;
};

class ServerAlwaysFailReader : public melon::ProgressiveReader {
public:
    ServerAlwaysFailReader(melon::Controller* cntl, google::protobuf::Closure* done)
        : _cntl(cntl)
        , _done(done) {}

    // @ProgressiveReader
    mutil::Status OnReadOnePart(const void* /*data*/, size_t /*length*/) {
        return mutil::Status(-1, "intended fail at %s:%d", __FILE__, __LINE__);
    }    

    void OnEndOfMessage(const mutil::Status& st) {
        melon::ClosureGuard done_guard(_done);
        CHECK_EQ(-1, st.error_code());
        _cntl->SetFailed("Must Failed");
        LOG(INFO) << "Destroy " << this << ": " << st;
        delete this;
    }
private:
    melon::Controller* _cntl;
    google::protobuf::Closure* _done;
};

class UploadServiceImpl : public ::test::UploadService {
public:
    void Upload(::google::protobuf::RpcController* controller,
                        const ::test::HttpRequest* request,
                        ::test::HttpResponse* response,
                        ::google::protobuf::Closure* done) {
        melon::Controller* cntl = static_cast<melon::Controller*>(controller);
        check_header(cntl);
        cntl->request_will_be_read_progressively();
        cntl->ReadProgressiveAttachmentBy(new ServerProgressiveReader(cntl, done));
    }
                        
    void UploadFailed(::google::protobuf::RpcController* controller,
                        const ::test::HttpRequest* request,
                        ::test::HttpResponse* response,
                        ::google::protobuf::Closure* done) {
        melon::Controller* cntl = static_cast<melon::Controller*>(controller);
        check_header(cntl);
        cntl->request_will_be_read_progressively();
        cntl->ReadProgressiveAttachmentBy(new ServerAlwaysFailReader(cntl, done));
    }

private:
    void check_header(melon::Controller* cntl) {
        const std::string* test_header = cntl->http_request().GetHeader(TEST_PROGRESSIVE_HEADER);
        GOOGLE_CHECK_NOTNULL(test_header);
        CHECK_EQ(*test_header, TEST_PROGRESSIVE_HEADER_VAL);
    }
};

TEST_F(HttpTest, server_end_read_short_body_progressively) {
    const int port = 8923;
    melon::ServiceOptions opt;
    opt.enable_progressive_read = true;
    opt.ownership = melon::SERVER_DOESNT_OWN_SERVICE;
    UploadServiceImpl upsvc;
    melon::Server server;
    EXPECT_EQ(0, server.AddService(&upsvc, opt));
    EXPECT_EQ(0, server.Start(port, NULL));

    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_HTTP;
    ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));
    melon::Controller cntl;
    cntl.http_request().uri() = "/UploadService/Upload";
    cntl.http_request().SetHeader(TEST_PROGRESSIVE_HEADER, TEST_PROGRESSIVE_HEADER_VAL);
    cntl.http_request().set_method(melon::HTTP_METHOD_POST);
    
    ASSERT_GT(PA_DATA_LEN, 8u);  // long enough to hold a 64-bit decimal.
    char buf[PA_DATA_LEN];
    for (size_t c = 0; c < 10000;) {
        CopyPAPrefixedWithSeqNo(buf, c);
        if (cntl.request_attachment().append(buf, sizeof(buf)) != 0) {
            if (errno == melon::EOVERCROWDED) {
                LOG(INFO) << "full msg=" << cntl.request_attachment().to_string();
            } else {
                LOG(INFO) << "Error:" << errno;
            }
            break;
        }
        ++c;
    }
    channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
    ASSERT_FALSE(cntl.Failed());
}

TEST_F(HttpTest, server_end_read_failed) {
    const int port = 8923;
    melon::ServiceOptions opt;
    opt.enable_progressive_read = true;
    opt.ownership = melon::SERVER_DOESNT_OWN_SERVICE;
    UploadServiceImpl upsvc;
    melon::Server server;
    EXPECT_EQ(0, server.AddService(&upsvc, opt));
    EXPECT_EQ(0, server.Start(port, NULL));

    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_HTTP;
    ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));
    melon::Controller cntl;
    cntl.http_request().uri() = "/UploadService/UploadFailed";
    cntl.http_request().SetHeader(TEST_PROGRESSIVE_HEADER, TEST_PROGRESSIVE_HEADER_VAL);
    cntl.http_request().set_method(melon::HTTP_METHOD_POST);

    ASSERT_GT(PA_DATA_LEN, 8u);  // long enough to hold a 64-bit decimal.
    char buf[PA_DATA_LEN];
    for (size_t c = 0; c < 10;) {
        CopyPAPrefixedWithSeqNo(buf, c);
        if (cntl.request_attachment().append(buf, sizeof(buf)) != 0) {
            if (errno == melon::EOVERCROWDED) {
                LOG(INFO) << "full msg=" << cntl.request_attachment().to_string();
            } else {
                LOG(INFO) << "Error:" << errno;
            }
            break;
        }
        ++c;
    }
    channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
    ASSERT_TRUE(cntl.Failed());
}

TEST_F(HttpTest, http2_sanity) {
    const int port = 8923;
    melon::Server server;
    EXPECT_EQ(0, server.AddService(&_svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, NULL));

    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = "h2";
    ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));

    // Check that the first request with size larger than the default window can
    // be sent out, when remote settings are not received.
    melon::Controller cntl;
    test::EchoRequest big_req;
    test::EchoResponse res;
    std::string message(2 * 1024 * 1024 /* 2M */, 'x');
    big_req.set_message(message);
    cntl.http_request().set_method(melon::HTTP_METHOD_POST);
    cntl.http_request().uri() = "/EchoService/Echo";
    channel.CallMethod(NULL, &cntl, &big_req, &res, NULL);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ(EXP_RESPONSE, res.message());

    // socket replacement when streamId runs out, the initial streamId is a special
    // value set in ctor of H2Context so that the number 15000 is enough to run out
    // of stream.
    test::EchoRequest req;
    req.set_message(EXP_REQUEST);
    for (int i = 0; i < 15000; ++i) {
        melon::Controller cntl;
        cntl.http_request().set_content_type("application/json");
        cntl.http_request().set_method(melon::HTTP_METHOD_POST);
        cntl.http_request().uri() = "/EchoService/Echo";
        channel.CallMethod(NULL, &cntl, &req, &res, NULL);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_EQ(EXP_RESPONSE, res.message());
    }

    // check connection window size
    melon::SocketUniquePtr main_ptr;
    melon::SocketUniquePtr agent_ptr;
    EXPECT_EQ(melon::Socket::Address(channel._server_id, &main_ptr), 0);
    EXPECT_EQ(main_ptr->GetAgentSocket(&agent_ptr, NULL), 0);
    melon::policy::H2Context* ctx = static_cast<melon::policy::H2Context*>(agent_ptr->parsing_context());
    ASSERT_GT(ctx->_remote_window_left.load(std::memory_order_relaxed),
             melon::H2Settings::DEFAULT_INITIAL_WINDOW_SIZE / 2);
}

TEST_F(HttpTest, http2_ping) {
    // This test injects PING frames before and after header and data.
    melon::Controller cntl;

    // Prepare request
    mutil::IOBuf req_out;
    int h2_stream_id = 0;
    MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);
    // Prepare response
    mutil::IOBuf res_out;
    char pingbuf[9 /*FRAME_HEAD_SIZE*/ + 8 /*Opaque Data*/];
    melon::policy::SerializeFrameHead(pingbuf, 8, melon::policy::H2_FRAME_PING, 0, 0);
    // insert ping before header and data
    res_out.append(pingbuf, sizeof(pingbuf));
    MakeH2EchoResponseBuf(&res_out, h2_stream_id);
    // insert ping after header and data
    res_out.append(pingbuf, sizeof(pingbuf));
    // parse response
    melon::ParseResult res_pr =
            melon::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, NULL);
    ASSERT_TRUE(res_pr.is_ok());
    // process response
    ProcessMessage(melon::policy::ProcessHttpResponse, res_pr.message(), false);
    ASSERT_FALSE(cntl.Failed());
}

inline void SaveUint32(void* out, uint32_t v) {
    uint8_t* p = (uint8_t*)out;
    p[0] = (v >> 24) & 0xFF;
    p[1] = (v >> 16) & 0xFF;
    p[2] = (v >> 8) & 0xFF;
    p[3] = v & 0xFF;
}

TEST_F(HttpTest, http2_rst_before_header) {
    melon::Controller cntl;
    // Prepare request
    mutil::IOBuf req_out;
    int h2_stream_id = 0;
    MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);
    // Prepare response
    mutil::IOBuf res_out;
    char rstbuf[9 /*FRAME_HEAD_SIZE*/ + 4];
    melon::policy::SerializeFrameHead(rstbuf, 4, melon::policy::H2_FRAME_RST_STREAM, 0, h2_stream_id);
    SaveUint32(rstbuf + 9, melon::H2_INTERNAL_ERROR);
    res_out.append(rstbuf, sizeof(rstbuf));
    MakeH2EchoResponseBuf(&res_out, h2_stream_id);
    // parse response
    melon::ParseResult res_pr =
            melon::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, NULL);
    ASSERT_TRUE(res_pr.is_ok());
    // process response
    ProcessMessage(melon::policy::ProcessHttpResponse, res_pr.message(), false);
    ASSERT_TRUE(cntl.Failed());
    ASSERT_TRUE(cntl.ErrorCode() == melon::EHTTP);
    ASSERT_TRUE(cntl.http_response().status_code() == melon::HTTP_STATUS_INTERNAL_SERVER_ERROR);
}

TEST_F(HttpTest, http2_rst_after_header_and_data) {
    melon::Controller cntl;
    // Prepare request
    mutil::IOBuf req_out;
    int h2_stream_id = 0;
    MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);
    // Prepare response
    mutil::IOBuf res_out;
    MakeH2EchoResponseBuf(&res_out, h2_stream_id);
    char rstbuf[9 /*FRAME_HEAD_SIZE*/ + 4];
    melon::policy::SerializeFrameHead(rstbuf, 4, melon::policy::H2_FRAME_RST_STREAM, 0, h2_stream_id);
    SaveUint32(rstbuf + 9, melon::H2_INTERNAL_ERROR);
    res_out.append(rstbuf, sizeof(rstbuf));
    // parse response
    melon::ParseResult res_pr =
            melon::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, NULL);
    ASSERT_TRUE(res_pr.is_ok());
    // process response
    ProcessMessage(melon::policy::ProcessHttpResponse, res_pr.message(), false);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(cntl.http_response().status_code() == melon::HTTP_STATUS_OK);
}

TEST_F(HttpTest, http2_window_used_up) {
    melon::Controller cntl;
    mutil::IOBuf request_buf;
    test::EchoRequest req;
    // longer message to trigger using up window size sooner
    req.set_message("FLOW_CONTROL_FLOW_CONTROL");
    cntl.http_request().set_method(melon::HTTP_METHOD_POST);
    cntl.http_request().set_content_type("application/proto");
    melon::policy::SerializeHttpRequest(&request_buf, &cntl, &req);

    char settingsbuf[melon::policy::FRAME_HEAD_SIZE + 36];
    melon::H2Settings h2_settings;
    const size_t nb = melon::policy::SerializeH2Settings(h2_settings, settingsbuf + melon::policy::FRAME_HEAD_SIZE);
    melon::policy::SerializeFrameHead(settingsbuf, nb, melon::policy::H2_FRAME_SETTINGS, 0, 0);
    mutil::IOBuf buf;
    buf.append(settingsbuf, melon::policy::FRAME_HEAD_SIZE + nb);
    melon::policy::ParseH2Message(&buf, _h2_client_sock.get(), false, NULL);

    int nsuc = melon::H2Settings::DEFAULT_INITIAL_WINDOW_SIZE / cntl.request_attachment().size();
    for (int i = 0; i <= nsuc; i++) {
        melon::policy::H2UnsentRequest* h2_req = melon::policy::H2UnsentRequest::New(&cntl);
        cntl._current_call.stream_user_data = h2_req;
        melon::SocketMessage* socket_message = NULL;
        melon::policy::PackH2Request(NULL, &socket_message, cntl.call_id().value,
                                    NULL, &cntl, request_buf, NULL);
        mutil::IOBuf dummy;
        mutil::Status st = socket_message->AppendAndDestroySelf(&dummy, _h2_client_sock.get());
        if (i == nsuc) {
            // the last message should fail according to flow control policy.
            ASSERT_FALSE(st.ok());
            ASSERT_TRUE(st.error_code() == melon::ELIMIT);
            ASSERT_TRUE(turbo::starts_with(st.error_str(), "remote_window_left is not enough"));
        } else {
            ASSERT_TRUE(st.ok());
        }
        h2_req->DestroyStreamUserData(_h2_client_sock, &cntl, 0, false);
    }
}

TEST_F(HttpTest, http2_settings) {
    char settingsbuf[melon::policy::FRAME_HEAD_SIZE + 36];
    melon::H2Settings h2_settings;
    h2_settings.header_table_size = 8192;
    h2_settings.max_concurrent_streams = 1024;
    h2_settings.stream_window_size= (1u << 29) - 1;
    const size_t nb = melon::policy::SerializeH2Settings(h2_settings, settingsbuf + melon::policy::FRAME_HEAD_SIZE);
    melon::policy::SerializeFrameHead(settingsbuf, nb, melon::policy::H2_FRAME_SETTINGS, 0, 0);
    mutil::IOBuf buf;
    buf.append(settingsbuf, melon::policy::FRAME_HEAD_SIZE + nb);

    melon::policy::H2Context* ctx = new melon::policy::H2Context(_socket.get(), NULL);
    CHECK_EQ(ctx->Init(), 0);
    _socket->initialize_parsing_context(&ctx);
    ctx->_conn_state = melon::policy::H2_CONNECTION_READY;
    // parse settings
    melon::policy::ParseH2Message(&buf, _socket.get(), false, NULL);

    mutil::IOPortal response_buf;
    CHECK_EQ(response_buf.append_from_file_descriptor(_pipe_fds[0], 1024),
             (ssize_t)melon::policy::FRAME_HEAD_SIZE);
    melon::policy::H2FrameHead frame_head;
    mutil::IOBufBytesIterator it(response_buf);
    ctx->ConsumeFrameHead(it, &frame_head);
    CHECK_EQ(frame_head.type, melon::policy::H2_FRAME_SETTINGS);
    CHECK_EQ(frame_head.flags, 0x01 /* H2_FLAGS_ACK */);
    CHECK_EQ(frame_head.stream_id, 0);
    ASSERT_TRUE(ctx->_remote_settings.header_table_size == 8192);
    ASSERT_TRUE(ctx->_remote_settings.max_concurrent_streams == 1024);
    ASSERT_TRUE(ctx->_remote_settings.stream_window_size == (1u << 29) - 1);
}

TEST_F(HttpTest, http2_invalid_settings) {
    {
        melon::Server server;
        melon::ServerOptions options;
        options.h2_settings.stream_window_size = melon::H2Settings::MAX_WINDOW_SIZE + 1;
        ASSERT_EQ(-1, server.Start("127.0.0.1:8924", &options));
    }
    {
        melon::Server server;
        melon::ServerOptions options;
        options.h2_settings.max_frame_size =
            melon::H2Settings::DEFAULT_MAX_FRAME_SIZE - 1;
        ASSERT_EQ(-1, server.Start("127.0.0.1:8924", &options));
    }
    {
        melon::Server server;
        melon::ServerOptions options;
        options.h2_settings.max_frame_size =
            melon::H2Settings::MAX_OF_MAX_FRAME_SIZE + 1;
        ASSERT_EQ(-1, server.Start("127.0.0.1:8924", &options));
    }
}

TEST_F(HttpTest, http2_not_closing_socket_when_rpc_timeout) {
    const int port = 8923;
    melon::Server server;
    EXPECT_EQ(0, server.AddService(&_svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, NULL));
    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = "h2";
    ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));

    test::EchoRequest req;
    test::EchoResponse res;
    req.set_message(EXP_REQUEST);
    {
        // make a successful call to create the connection first
        melon::Controller cntl;
        cntl.http_request().set_method(melon::HTTP_METHOD_POST);
        cntl.http_request().uri() = "/EchoService/Echo";
        channel.CallMethod(NULL, &cntl, &req, &res, NULL);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_EQ(EXP_RESPONSE, res.message());
    }

    melon::SocketUniquePtr main_ptr;
    EXPECT_EQ(melon::Socket::Address(channel._server_id, &main_ptr), 0);
    melon::SocketId agent_id = main_ptr->_agent_socket_id.load(std::memory_order_relaxed);

    for (int i = 0; i < 4; i++) {
        melon::Controller cntl;
        cntl.set_timeout_ms(50);
        cntl.http_request().set_method(melon::HTTP_METHOD_POST);
        cntl.http_request().uri() = "/EchoService/Echo?sleep_ms=300";
        channel.CallMethod(NULL, &cntl, &req, &res, NULL);
        ASSERT_TRUE(cntl.Failed());

        melon::SocketUniquePtr ptr;
        melon::SocketId id = main_ptr->_agent_socket_id.load(std::memory_order_relaxed);
        EXPECT_EQ(id, agent_id);
    }

    {
        // make a successful call again to make sure agent_socket not changing
        melon::Controller cntl;
        cntl.http_request().set_method(melon::HTTP_METHOD_POST);
        cntl.http_request().uri() = "/EchoService/Echo";
        channel.CallMethod(NULL, &cntl, &req, &res, NULL);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_EQ(EXP_RESPONSE, res.message());
        melon::SocketUniquePtr ptr;
        melon::SocketId id = main_ptr->_agent_socket_id.load(std::memory_order_relaxed);
        EXPECT_EQ(id, agent_id);
    }
}

TEST_F(HttpTest, http2_header_after_data) {
    melon::Controller cntl;

    // Prepare request
    mutil::IOBuf req_out;
    int h2_stream_id = 0;
    MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);

    // Prepare response to res_out
    mutil::IOBuf res_out;
    {
        mutil::IOBuf data_buf;
        test::EchoResponse res;
        res.set_message(EXP_RESPONSE);
        {
            mutil::IOBufAsZeroCopyOutputStream wrapper(&data_buf);
            EXPECT_TRUE(res.SerializeToZeroCopyStream(&wrapper));
        }
        melon::policy::H2Context* ctx =
            static_cast<melon::policy::H2Context*>(_h2_client_sock->parsing_context());
        melon::HPacker& hpacker = ctx->hpacker();
        mutil::IOBufAppender header1_appender;
        melon::HPackOptions options;
        options.encode_name = false;    /* disable huffman encoding */
        options.encode_value = false;
        {
            melon::HPacker::Header header(":status", "200");
            hpacker.Encode(&header1_appender, header, options);
        }
        {
            melon::HPacker::Header header("content-length",
                    turbo::str_format("%llu", (unsigned long long)data_buf.size()));
            hpacker.Encode(&header1_appender, header, options);
        }
        {
            melon::HPacker::Header header(":status", "200");
            hpacker.Encode(&header1_appender, header, options);
        }
        {
            melon::HPacker::Header header("content-type", "application/proto");
            hpacker.Encode(&header1_appender, header, options);
        }
        {
            melon::HPacker::Header header("user-defined1", "a");
            hpacker.Encode(&header1_appender, header, options);
        }
        mutil::IOBuf header1;
        header1_appender.move_to(header1);

        char headbuf[melon::policy::FRAME_HEAD_SIZE];
        melon::policy::SerializeFrameHead(headbuf, header1.size(),
                melon::policy::H2_FRAME_HEADERS, 0, h2_stream_id);
        // append header1
        res_out.append(headbuf, sizeof(headbuf));
        res_out.append(mutil::IOBuf::Movable(header1));

        melon::policy::SerializeFrameHead(headbuf, data_buf.size(),
            melon::policy::H2_FRAME_DATA, 0, h2_stream_id);
        // append data
        res_out.append(headbuf, sizeof(headbuf));
        res_out.append(mutil::IOBuf::Movable(data_buf));

        mutil::IOBufAppender header2_appender;
        {
            melon::HPacker::Header header("user-defined1", "overwrite-a");
            hpacker.Encode(&header2_appender, header, options);
        }
        {
            melon::HPacker::Header header("user-defined2", "b");
            hpacker.Encode(&header2_appender, header, options);
        }
        mutil::IOBuf header2;
        header2_appender.move_to(header2);

        melon::policy::SerializeFrameHead(headbuf, header2.size(),
                melon::policy::H2_FRAME_HEADERS, 0x05/* end header and stream */,
                h2_stream_id);
        // append header2
        res_out.append(headbuf, sizeof(headbuf));
        res_out.append(mutil::IOBuf::Movable(header2));
    }
    // parse response
    melon::ParseResult res_pr =
            melon::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, NULL);
    ASSERT_TRUE(res_pr.is_ok());
    // process response
    ProcessMessage(melon::policy::ProcessHttpResponse, res_pr.message(), false);
    ASSERT_FALSE(cntl.Failed());

    melon::HttpHeader& res_header = cntl.http_response();
    ASSERT_EQ(res_header.content_type(), "application/proto");
    // Check overlapped header is overwritten by the latter.
    const std::string* user_defined1 = res_header.GetHeader("user-defined1");
    ASSERT_EQ(*user_defined1, "a,overwrite-a");
    const std::string* user_defined2 = res_header.GetHeader("user-defined2");
    ASSERT_EQ(*user_defined2, "b");
}

TEST_F(HttpTest, http2_goaway_sanity) {
    melon::Controller cntl;
    // Prepare request
    mutil::IOBuf req_out;
    int h2_stream_id = 0;
    MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);
    // Prepare response
    mutil::IOBuf res_out;
    MakeH2EchoResponseBuf(&res_out, h2_stream_id);
    // append goaway
    char goawaybuf[9 /*FRAME_HEAD_SIZE*/ + 8];
    melon::policy::SerializeFrameHead(goawaybuf, 8, melon::policy::H2_FRAME_GOAWAY, 0, 0);
    SaveUint32(goawaybuf + 9, 0x7fffd8ef /*last stream id*/);
    SaveUint32(goawaybuf + 13, melon::H2_NO_ERROR);
    res_out.append(goawaybuf, sizeof(goawaybuf));
    // parse response
    melon::ParseResult res_pr =
            melon::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, NULL);
    ASSERT_TRUE(res_pr.is_ok());
    // process response
    ProcessMessage(melon::policy::ProcessHttpResponse, res_pr.message(), false);
    ASSERT_TRUE(!cntl.Failed());

    // parse GOAWAY
    res_pr = melon::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, NULL);
    ASSERT_EQ(res_pr.error(), melon::PARSE_ERROR_NOT_ENOUGH_DATA);

    // Since GOAWAY has been received, the next request should fail
    melon::policy::H2UnsentRequest* h2_req = melon::policy::H2UnsentRequest::New(&cntl);
    cntl._current_call.stream_user_data = h2_req;
    melon::SocketMessage* socket_message = NULL;
    melon::policy::PackH2Request(NULL, &socket_message, cntl.call_id().value,
                                NULL, &cntl, mutil::IOBuf(), NULL);
    mutil::IOBuf dummy;
    mutil::Status st = socket_message->AppendAndDestroySelf(&dummy, _h2_client_sock.get());
    ASSERT_EQ(st.error_code(), melon::ELOGOFF);
    ASSERT_TRUE(st.error_data().ends_with("the connection just issued GOAWAY"));
}

class AfterRecevingGoAway : public ::google::protobuf::Closure {
public:
    void Run() {
        ASSERT_EQ(melon::EHTTP, cntl.ErrorCode());
        delete this;
    }
    melon::Controller cntl;
};

TEST_F(HttpTest, http2_handle_goaway_streams) {
    const mutil::EndPoint ep(mutil::IP_ANY, 5961);
    mutil::fd_guard listenfd(mutil::tcp_listen(ep));
    ASSERT_GT(listenfd, 0);

    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_H2;
    ASSERT_EQ(0, channel.Init(ep, &options));

    int req_size = 10;
    std::vector<melon::CallId> ids(req_size);
    for (int i = 0; i < req_size; i++) {
        AfterRecevingGoAway* done = new AfterRecevingGoAway;
        melon::Controller& cntl = done->cntl;
        ids.push_back(cntl.call_id());
        cntl.set_timeout_ms(-1);
        cntl.http_request().uri() = "/it-doesnt-matter";
        channel.CallMethod(NULL, &cntl, NULL, NULL, done);
    }

    int servfd = accept(listenfd, NULL, NULL);
    ASSERT_GT(servfd, 0);
    // Sleep for a while to make sure that server has received all data.
    fiber_usleep(2000);
    char goawaybuf[melon::policy::FRAME_HEAD_SIZE + 8];
    SerializeFrameHead(goawaybuf, 8, melon::policy::H2_FRAME_GOAWAY, 0, 0);
    SaveUint32(goawaybuf + melon::policy::FRAME_HEAD_SIZE, 0);
    SaveUint32(goawaybuf + melon::policy::FRAME_HEAD_SIZE + 4, 0);
    ASSERT_EQ((ssize_t)melon::policy::FRAME_HEAD_SIZE + 8, ::write(servfd, goawaybuf, melon::policy::FRAME_HEAD_SIZE + 8));

    // After receving GOAWAY, the callbacks in client should be run correctly.
    for (int i = 0; i < req_size; i++) {
        melon::Join(ids[i]);
    }
}

TEST_F(HttpTest, spring_protobuf_content_type) {
    const int port = 8923;
    melon::Server server;
    EXPECT_EQ(0, server.AddService(&_svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, nullptr));

    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = "http";
    ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));

    melon::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    req.set_message(EXP_REQUEST);
    cntl.http_request().set_method(melon::HTTP_METHOD_POST);
    cntl.http_request().uri() = "/EchoService/Echo";
    cntl.http_request().set_content_type("application/x-protobuf");
    cntl.request_attachment().append(req.SerializeAsString());
    channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ("application/x-protobuf", cntl.http_response().content_type());
    ASSERT_TRUE(res.ParseFromString(cntl.response_attachment().to_string()));
    ASSERT_EQ(EXP_RESPONSE, res.message());

    melon::Controller cntl2;
    test::EchoService_Stub stub(&channel);
    req.set_message(EXP_REQUEST);
    res.Clear();
    cntl2.http_request().set_content_type("application/x-protobuf");
    stub.Echo(&cntl2, &req, &res, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ(EXP_RESPONSE, res.message());
    ASSERT_EQ("application/x-protobuf", cntl.http_response().content_type());
}

TEST_F(HttpTest, dump_http_request) {
    // save origin value of gflag
    auto rpc_dump_dir = melon::FLAGS_rpc_dump_dir;
    auto rpc_dump_max_requests_in_one_file = melon::FLAGS_rpc_dump_max_requests_in_one_file;

    // set gflag and global variable in order to be sure to dump request
    melon::FLAGS_rpc_dump = true;
    melon::FLAGS_rpc_dump_dir = "dump_http_request";
    melon::FLAGS_rpc_dump_max_requests_in_one_file = 1;
    melon::g_rpc_dump_sl.ever_grabbed = true;
    melon::g_rpc_dump_sl.sampling_range = melon::var::COLLECTOR_SAMPLING_BASE;

    // init channel
    const int port = 8923;
    melon::Server server;
    EXPECT_EQ(0, server.AddService(&_svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, nullptr));

    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = "http";
    ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));

    // send request and dump it to file
    {
        test::EchoRequest req;
        req.set_message(EXP_REQUEST);
        std::string req_json;
        ASSERT_TRUE(json2pb::ProtoMessageToJson(req, &req_json));

        melon::Controller cntl;
        cntl.http_request().uri() = "/EchoService/Echo";
        cntl.http_request().set_content_type("application/json");
        cntl.http_request().set_method(melon::HTTP_METHOD_POST);
        cntl.request_attachment() = req_json;
        channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
        ASSERT_FALSE(cntl.Failed());

        // sleep 1s, because rpc_dump doesn't run immediately
        sleep(1);
    }

    // replay request from dump file
    {
        melon::SampleIterator it(melon::FLAGS_rpc_dump_dir);
        melon::SampledRequest* sample = it.Next();
        ASSERT_NE(nullptr, sample);

        std::unique_ptr<melon::SampledRequest> sample_guard(sample);

        // the logic of next code is same as that in rpc_replay.cpp
        ASSERT_EQ(sample->meta.protocol_type(), melon::PROTOCOL_HTTP);
        melon::Controller cntl;
        cntl.reset_sampled_request(sample_guard.release());
        melon::HttpMessage http_message;
        http_message.ParseFromIOBuf(sample->request);
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
    mutil::DeleteFile(mutil::FilePath(melon::FLAGS_rpc_dump_dir), true);

    // restore gflag and global variable
    melon::FLAGS_rpc_dump = false;
    melon::FLAGS_rpc_dump_dir = rpc_dump_dir;
    melon::FLAGS_rpc_dump_max_requests_in_one_file = rpc_dump_max_requests_in_one_file;
    melon::g_rpc_dump_sl.ever_grabbed = false;
    melon::g_rpc_dump_sl.sampling_range = 0;
}

TEST_F(HttpTest, spring_protobuf_text_content_type) {
    const int port = 8923;
    melon::Server server;
    EXPECT_EQ(0, server.AddService(&_svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, nullptr));

    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = "http";
    ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));

    melon::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    req.set_message(EXP_REQUEST);
    cntl.http_request().set_method(melon::HTTP_METHOD_POST);
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

class HttpServiceImpl : public ::test::HttpService {
    public:
    void Head(::google::protobuf::RpcController* cntl_base,
        const ::test::HttpRequest*,
        ::test::HttpResponse*,
        ::google::protobuf::Closure* done) override {
        melon::ClosureGuard done_guard(done);
        melon::Controller* cntl = static_cast<melon::Controller*>(cntl_base);
        ASSERT_EQ(cntl->http_request().method(), melon::HTTP_METHOD_HEAD);
        const std::string* index = cntl->http_request().GetHeader("x-db-index");
        ASSERT_NE(nullptr, index);
        int i;
        ASSERT_TRUE(mutil::StringToInt(*index, &i));
        cntl->http_response().set_content_type("text/plain");
        if (i % 2 == 0) {
            cntl->http_response().SetHeader("Content-Length",
                EXP_RESPONSE_CONTENT_LENGTH);
        } else {
            cntl->http_response().SetHeader("Transfer-Encoding",
                EXP_RESPONSE_TRANSFER_ENCODING);
        }
    }

    void Expect(::google::protobuf::RpcController* cntl_base,
        const ::test::HttpRequest*,
        ::test::HttpResponse*,
        ::google::protobuf::Closure* done) override {
        melon::ClosureGuard done_guard(done);
        melon::Controller* cntl = static_cast<melon::Controller*>(cntl_base);
        const std::string* expect = cntl->http_request().GetHeader("Expect");
        ASSERT_TRUE(expect != NULL);
        ASSERT_EQ("100-continue", *expect);
        ASSERT_EQ(cntl->http_request().method(), melon::HTTP_METHOD_POST);
        cntl->response_attachment().append("world");
    }
};

TEST_F(HttpTest, http_head) {
    const int port = 8923;
    melon::Server server;
    HttpServiceImpl svc;
    EXPECT_EQ(0, server.AddService(&svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, NULL));

    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_HTTP;
    ASSERT_EQ(0, channel.Init(mutil::EndPoint(mutil::my_ip(), port), &options));
    for (int i = 0; i < 100; ++i) {
        melon::Controller cntl;
        cntl.http_request().set_method(melon::HTTP_METHOD_HEAD);
        cntl.http_request().uri().set_path("/HttpService/Head");
        cntl.http_request().SetHeader("x-db-index", mutil::IntToString(i));
        channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);

        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        if (i % 2 == 0) {
            const std::string* content_length
                = cntl.http_response().GetHeader("content-length");
            ASSERT_NE(nullptr, content_length);
            ASSERT_EQ(EXP_RESPONSE_CONTENT_LENGTH, *content_length);
        } else {
            const std::string* transfer_encoding
                = cntl.http_response().GetHeader("Transfer-Encoding");
            ASSERT_NE(nullptr, transfer_encoding);
            ASSERT_EQ(EXP_RESPONSE_TRANSFER_ENCODING, *transfer_encoding);
        }
    }
}

#define MELON_CRLF "\r\n"

void MakeHttpRequestHeaders(mutil::IOBuf* out,
                            melon::HttpHeader* h,
                            const mutil::EndPoint& remote_side) {
    mutil::IOBufBuilder os;
    os << HttpMethod2Str(h->method()) << ' ';
    const melon::URI& uri = h->uri();
    uri.PrintWithoutHost(os); // host is sent by "Host" header.
    os << " HTTP/" << h->major_version() << '.'
       << h->minor_version() << MELON_CRLF;
    //rfc 7230#section-5.4:
    //A client MUST send a Host header field in all HTTP/1.1 request
    //messages. If the authority component is missing or undefined for
    //the target URI, then a client MUST send a Host header field with an
    //empty field-value.
    //rfc 7231#sec4.3:
    //the request-target consists of only the host name and port number of
    //the tunnel destination, seperated by a colon. For example,
    //Host: server.example.com:80
    if (h->GetHeader("host") == NULL) {
        os << "Host: ";
        if (!uri.host().empty()) {
            os << uri.host();
            if (uri.port() >= 0) {
                os << ':' << uri.port();
            }
        } else if (remote_side.port != 0) {
            os << remote_side;
        }
        os << MELON_CRLF;
    }
    if (!h->content_type().empty()) {
        os << "Content-Type: " << h->content_type()
           << MELON_CRLF;
    }
    for (melon::HttpHeader::HeaderIterator it = h->HeaderBegin();
         it != h->HeaderEnd(); ++it) {
        os << it->first << ": " << it->second << MELON_CRLF;
    }
    if (h->GetHeader("Accept") == NULL) {
        os << "Accept: */*" MELON_CRLF;
    }
    // The fake "curl" user-agent may let servers return plain-text results.
    if (h->GetHeader("User-Agent") == NULL) {
        os << "User-Agent: melon/1.0 curl/7.0" MELON_CRLF;
    }
    const std::string& user_info = h->uri().user_info();
    if (!user_info.empty() && h->GetHeader("Authorization") == NULL) {
        // NOTE: just assume user_info is well formatted, namely
        // "<user_name>:<password>". Users are very unlikely to add extra
        // characters in this part and even if users did, most of them are
        // invalid and rejected by http_parser_parse_url().
        std::string encoded_user_info;
        turbo::base64_encode(user_info, &encoded_user_info);
        os << "Authorization: Basic " << encoded_user_info << MELON_CRLF;
    }
    os << MELON_CRLF;  // CRLF before content
    os.move_to(*out);
}

#undef MELON_CRLF

void ReadOneResponse(melon::SocketUniquePtr& sock,
    melon::DestroyingPtr<melon::policy::HttpContext>& imsg_guard) {
#if defined(OS_LINUX)
    ASSERT_EQ(0, fiber_fd_wait(sock->fd(), EPOLLIN));
#elif defined(OS_MACOSX)
    ASSERT_EQ(0, fiber_fd_wait(sock->fd(), EVFILT_READ));
#endif

    mutil::IOPortal read_buf;
    int64_t start_time = mutil::gettimeofday_us();
    while (true) {
        const ssize_t nr = read_buf.append_from_file_descriptor(sock->fd(), 4096);
        LOG(INFO) << "nr=" << nr;
        LOG(INFO) << mutil::ToPrintableString(read_buf);
        ASSERT_TRUE(nr > 0 || (nr < 0 && errno == EAGAIN));
        if (errno == EAGAIN) {
            ASSERT_LT(mutil::gettimeofday_us(), start_time + 1000000L) << "Too long!";
            fiber_usleep(1000);
            continue;
        }
        melon::ParseResult pr = melon::policy::ParseHttpMessage(&read_buf, sock.get(), false, NULL);
        ASSERT_TRUE(pr.error() == melon::PARSE_ERROR_NOT_ENOUGH_DATA || pr.is_ok());
        if (pr.is_ok()) {
            imsg_guard.reset(static_cast<melon::policy::HttpContext*>(pr.message()));
            break;
        }
    }
    ASSERT_TRUE(read_buf.empty());
}

TEST_F(HttpTest, http_expect) {
    const int port = 8923;
    melon::Server server;
    HttpServiceImpl svc;
    EXPECT_EQ(0, server.AddService(&svc, melon::SERVER_DOESNT_OWN_SERVICE));
    EXPECT_EQ(0, server.Start(port, NULL));

    mutil::EndPoint ep;
    ASSERT_EQ(0, mutil::str2endpoint("127.0.0.1:8923", &ep));
    melon::SocketOptions options;
    options.remote_side = ep;
    melon::SocketId id;
    ASSERT_EQ(0, melon::Socket::Create(options, &id));
    melon::SocketUniquePtr sock;
    ASSERT_EQ(0, melon::Socket::Address(id, &sock));

    mutil::IOBuf content;
    content.append("hello");
    melon::HttpHeader header;
    header.set_method(melon::HTTP_METHOD_POST);
    header.uri().set_path("/HttpService/Expect");
    header.SetHeader("Expect", "100-continue");
    header.SetHeader("Content-Length", std::to_string(content.size()));
    mutil::IOBuf header_buf;
    MakeHttpRequestHeaders(&header_buf, &header, ep);
    LOG(INFO) << mutil::ToPrintableString(header_buf);
    mutil::IOBuf request_buf(header_buf);
    request_buf.append(content);

    ASSERT_EQ(0, sock->Write(&header_buf));
    int64_t start_time = mutil::gettimeofday_us();
    while (sock->fd() < 0) {
        fiber_usleep(1000);
        ASSERT_LT(mutil::gettimeofday_us(), start_time + 1000000L) << "Too long!";
    }
    // 100 Continue
    melon::DestroyingPtr<melon::policy::HttpContext> imsg_guard;
    ReadOneResponse(sock, imsg_guard);
    ASSERT_EQ(imsg_guard->header().status_code(), melon::HTTP_STATUS_CONTINUE);

    ASSERT_EQ(0, sock->Write(&content));
    // 200 Ok
    ReadOneResponse(sock, imsg_guard);
    ASSERT_EQ(imsg_guard->header().status_code(), melon::HTTP_STATUS_OK);

    ASSERT_EQ(0, sock->Write(&request_buf));
    // 200 Ok
    ReadOneResponse(sock, imsg_guard);
    ASSERT_EQ(imsg_guard->header().status_code(), melon::HTTP_STATUS_OK);
}

} //namespace
