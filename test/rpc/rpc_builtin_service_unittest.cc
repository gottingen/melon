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

#include <sys/types.h>
#include <sys/socket.h>
#include <fstream>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <google/protobuf/descriptor.h>
#include "melon/utility/gperftools_profiler.h"
#include "melon/utility/time.h"
#include "melon/utility/macros.h"
#include "melon/rpc/socket.h"
#include "melon/rpc/server.h"
#include "melon/rpc/channel.h"
#include "melon/rpc/controller.h"
#include "melon/rpc/span.h"
#include "melon/rpc/reloadable_flags.h"
#include "melon/builtin/version_service.h"
#include "melon/builtin/health_service.h"
#include "melon/builtin/list_service.h"
#include "melon/builtin/status_service.h"
#include "melon/builtin/threads_service.h"
#include "melon/builtin/vlog_service.h"
#include "melon/builtin/index_service.h"        // IndexService
#include "melon/builtin/connections_service.h"  // ConnectionsService
#include "melon/builtin/flags_service.h"        // FlagsService
#include "melon/builtin/vars_service.h"         // VarsService
#include "melon/builtin/rpcz_service.h"         // RpczService
#include "melon/builtin/dir_service.h"          // DirService
#include "melon/builtin/pprof_service.h"        // PProfService
#include "melon/builtin/fibers_service.h"     // FibersService
#include "melon/builtin/ids_service.h"          // IdsService
#include "melon/builtin/sockets_service.h"      // SocketsService
#include "melon/builtin/memory_service.h"
#include "melon/builtin/common.h"
#include "melon/builtin/bad_method_service.h"
#include "../echo.pb.h"
#include "melon/proto/rpc/grpc_health_check.pb.h"
#include "melon/json2pb/pb_to_json.h"

DEFINE_bool(foo, false, "Flags for UT");
MELON_VALIDATE_GFLAG(foo, melon::PassValidate);

namespace melon {
DECLARE_bool(enable_rpcz);
DECLARE_bool(rpcz_hex_log_id);
DECLARE_int32(idle_timeout_second);
} // namespace rpc

int main(int argc, char* argv[]) {
    melon::FLAGS_idle_timeout_second = 0;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class EchoServiceImpl : public test::EchoService {
public:
    EchoServiceImpl() {}
    virtual ~EchoServiceImpl() {}
    virtual void Echo(google::protobuf::RpcController* cntl_base,
                      const test::EchoRequest* req,
                      test::EchoResponse* res,
                      google::protobuf::Closure* done) {
        melon::Controller* cntl =
                static_cast<melon::Controller*>(cntl_base);
        melon::ClosureGuard done_guard(done);
        TRACEPRINTF("MyAnnotation: %ld", cntl->log_id());
        if (req->sleep_us() > 0) {
            fiber_usleep(req->sleep_us());
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%" PRIu64, cntl->trace_id());
        res->set_message(buf);
    }
};

class ClosureChecker : public google::protobuf::Closure {
public:
    ClosureChecker() : _count(1) {}
    ~ClosureChecker() { EXPECT_EQ(0, _count); }
    
    void Run() {
        --_count;
    }
    
private:
    int _count;
};

void MyVLogSite() {
    VLOG(3) << "This is a VLOG!";
}

void CheckContent(const melon::Controller& cntl, const char* name) {
    const std::string& content = cntl.response_attachment().to_string();
    std::size_t pos = content.find(name);
    ASSERT_TRUE(pos != std::string::npos) << "name=" << name;
}

void CheckErrorText(const melon::Controller& cntl, const char* error) {
    std::size_t pos = cntl.ErrorText().find(error);
    ASSERT_TRUE(pos != std::string::npos) << "error=" << error;
}

void CheckFieldInContent(const melon::Controller& cntl,
                         const char* name, int32_t expect) {
    const std::string& content = cntl.response_attachment().to_string();
    std::size_t pos = content.find(name);
    ASSERT_TRUE(pos != std::string::npos);

    int32_t val = 0;
    ASSERT_EQ(1, sscanf(content.c_str() + pos + strlen(name), "%d", &val));
    ASSERT_EQ(expect, val) << "name=" << name;
}

void CheckAnnotation(const melon::Controller& cntl, int64_t expect) {
    const std::string& content = cntl.response_attachment().to_string();
    std::string expect_str;
    mutil::string_printf(&expect_str, "MyAnnotation: %" PRId64, expect);
    std::size_t pos = content.find(expect_str);
    ASSERT_TRUE(pos != std::string::npos) << expect;
}

void CheckTraceId(const melon::Controller& cntl,
                  const std::string& expect_id_str) {
    const std::string& content = cntl.response_attachment().to_string();
    std::string expect_str = std::string(melon::TRACE_ID_STR) + "=" + expect_id_str;
    std::size_t pos = content.find(expect_str);
    ASSERT_TRUE(pos != std::string::npos) << expect_str;
}

class BuiltinServiceTest : public ::testing::Test{
protected:
    BuiltinServiceTest(){};
    virtual ~BuiltinServiceTest(){};
    virtual void SetUp() { EXPECT_EQ(0, _server.AddBuiltinServices()); }
    virtual void TearDown() { StopAndJoin(); }

    void StopAndJoin() {
        _server.Stop(0);
        _server.Join();
        _server.ClearServices();
    }

    void SetUpController(melon::Controller* cntl, bool use_html) const {
        cntl->_server = &_server;
        if (use_html) {
            cntl->http_request().SetHeader(
                melon::USER_AGENT_STR, "just keep user agent non-empty");
        }
    }

    void TestIndex(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        melon::IndexService service;
        melon::IndexRequest req;
        melon::IndexResponse res;
        melon::Controller cntl;
        ClosureChecker done;
        SetUpController(&cntl, use_html);
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        EXPECT_EQ(expect_type, cntl.http_response().content_type());
    }

    void TestStatus(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        melon::StatusService service;
        melon::StatusRequest req;
        melon::StatusResponse res;
        melon::Controller cntl;
        ClosureChecker done;
        SetUpController(&cntl, use_html);
        EchoServiceImpl echo_svc;
        ASSERT_EQ(0, _server.AddService(
            &echo_svc, melon::SERVER_DOESNT_OWN_SERVICE));
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        EXPECT_EQ(expect_type, cntl.http_response().content_type());
        ASSERT_EQ(0, _server.RemoveService(&echo_svc));
    }
    
    void TestVLog(bool use_html) {
#if !MELON_WITH_GLOG
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        melon::VLogService service;
        melon::VLogRequest req;
        melon::VLogResponse res;
        melon::Controller cntl;
        ClosureChecker done;
        SetUpController(&cntl, use_html);
        MyVLogSite();
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        EXPECT_EQ(expect_type, cntl.http_response().content_type());
        CheckContent(cntl, "rpc_builtin_service_unittest");
#endif
    }
    
    void TestConnections(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        melon::ConnectionsService service;
        melon::ConnectionsRequest req;
        melon::ConnectionsResponse res;
        melon::Controller cntl;
        ClosureChecker done;
        SetUpController(&cntl, use_html);
        mutil::EndPoint ep;
        ASSERT_EQ(0, str2endpoint("127.0.0.1:9798", &ep));
        ASSERT_EQ(0, _server.Start(ep, nullptr));
        int self_port = -1;
        const int cfd = tcp_connect(ep, &self_port);
        ASSERT_GT(cfd, 0);
        char buf[64];
        snprintf(buf, sizeof(buf), "127.0.0.1:%d", self_port);
        usleep(100000);
        
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        EXPECT_EQ(expect_type, cntl.http_response().content_type());
        CheckContent(cntl, buf);
        CheckFieldInContent(cntl, "channel_connection_count: ", 0);

        close(cfd);
        StopAndJoin();
    }
    
    void TestBadMethod(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        melon::BadMethodService service;
        melon::BadMethodResponse res;
        {
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            melon::BadMethodRequest req;
            req.set_service_name(
                melon::PProfService::descriptor()->full_name());
            service.no_method(&cntl, &req, &res, &done);
            EXPECT_EQ(melon::ENOMETHOD, cntl.ErrorCode());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            CheckErrorText(cntl, "growth");
        }
    }

    void TestFlags(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        melon::FlagsService service;
        melon::FlagsRequest req;
        melon::FlagsResponse res;
        {
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            CheckContent(cntl, "fiber_concurrency");
        }
        {
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request()._unresolved_path = "foo";
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            CheckContent(cntl, "false");
        }
        {
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request()._unresolved_path = "foo";
            cntl.http_request().uri()
                    .SetQuery(melon::SETVALUE_STR, "true");
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
        }
        {
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request()._unresolved_path = "foo";
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            CheckContent(cntl, "true");
        }
    }

    void TestRpcz(bool enable, bool hex, bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        melon::RpczService service;
        melon::RpczRequest req;
        melon::RpczResponse res;
        if (!enable) {
            {
                ClosureChecker done;
                melon::Controller cntl;
                SetUpController(&cntl, use_html);
                service.disable(&cntl, &req, &res, &done);
                EXPECT_FALSE(cntl.Failed());
                EXPECT_FALSE(melon::FLAGS_enable_rpcz);
            }
            {
                ClosureChecker done;
                melon::Controller cntl;
                SetUpController(&cntl, use_html);
                service.default_method(&cntl, &req, &res, &done);
                EXPECT_FALSE(cntl.Failed());
                EXPECT_EQ(expect_type,
                          cntl.http_response().content_type());
                if (!use_html) {
                    CheckContent(cntl, "rpcz is not enabled");
                }
            }
            {
                ClosureChecker done;
                melon::Controller cntl;
                SetUpController(&cntl, use_html);
                service.stats(&cntl, &req, &res, &done);
                EXPECT_FALSE(cntl.Failed());
                if (!use_html) {
                    CheckContent(cntl, "rpcz is not enabled");
                }
            }
            return;
        }

        {
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            service.enable(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            EXPECT_TRUE(melon::FLAGS_enable_rpcz);
        }

        if (hex) {
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            service.hex_log_id(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_TRUE(melon::FLAGS_rpcz_hex_log_id);
        } else {
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            service.dec_log_id(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_FALSE(melon::FLAGS_rpcz_hex_log_id);
        }
        
        ASSERT_EQ(0, _server.AddService(new EchoServiceImpl(),
                                        melon::SERVER_OWNS_SERVICE));
        mutil::EndPoint ep;
        ASSERT_EQ(0, str2endpoint("127.0.0.1:9748", &ep));
        ASSERT_EQ(0, _server.Start(ep, nullptr));
        melon::Channel channel;
        ASSERT_EQ(0, channel.Init(ep, nullptr));
        test::EchoService_Stub stub(&channel);
        int64_t log_id = 1234567890;
        char querystr_buf[128];
        // Since LevelDB is unstable on jerkins, disable all the assertions here
        {
            // Find by trace_id
            test::EchoRequest echo_req;
            test::EchoResponse echo_res;
            melon::Controller echo_cntl;
            echo_req.set_message("hello");
            echo_cntl.set_log_id(++log_id);
            stub.Echo(&echo_cntl, &echo_req, &echo_res, nullptr);
            EXPECT_FALSE(echo_cntl.Failed());

            // Wait for level db to commit span information
            usleep(500000);
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request().uri()
                    .SetQuery(melon::TRACE_ID_STR, echo_res.message());
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            // CheckAnnotation(cntl, log_id);
        }

        {
            // Find by latency
            test::EchoRequest echo_req;
            test::EchoResponse echo_res;
            melon::Controller echo_cntl;
            echo_req.set_message("hello");
            echo_req.set_sleep_us(150000);
            echo_cntl.set_log_id(++log_id);
            stub.Echo(&echo_cntl, &echo_req, &echo_res, nullptr);
            EXPECT_FALSE(echo_cntl.Failed());

            // Wait for level db to commit span information
            usleep(500000);
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request().uri()
                    .SetQuery(melon::MIN_LATENCY_STR, "100000");
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            // CheckTraceId(cntl, echo_res.message());
        }

        {
            // Find by request size
            test::EchoRequest echo_req;
            test::EchoResponse echo_res;
            melon::Controller echo_cntl;
            std::string request_str(1500, 'a');
            echo_req.set_message(request_str);
            echo_cntl.set_log_id(++log_id);
            stub.Echo(&echo_cntl, &echo_req, &echo_res, nullptr);
            EXPECT_FALSE(echo_cntl.Failed());

            // Wait for level db to commit span information
            usleep(500000);
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request().uri()
                    .SetQuery(melon::MIN_REQUEST_SIZE_STR, "1024");
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            // CheckTraceId(cntl, echo_res.message());
        }

        {
            // Find by log id
            test::EchoRequest echo_req;
            test::EchoResponse echo_res;
            melon::Controller echo_cntl;
            echo_req.set_message("hello");
            echo_cntl.set_log_id(++log_id);
            stub.Echo(&echo_cntl, &echo_req, &echo_res, nullptr);
            EXPECT_FALSE(echo_cntl.Failed());

            // Wait for level db to commit span information
            usleep(500000);
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            snprintf(querystr_buf, sizeof(querystr_buf), "%" PRId64, log_id);
            cntl.http_request().uri()
                    .SetQuery(melon::LOG_ID_STR, querystr_buf);
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            // CheckTraceId(cntl, echo_res.message());
        }

        {
            ClosureChecker done;
            melon::Controller cntl;
            SetUpController(&cntl, use_html);
            service.stats(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            // CheckContent(cntl, "rpcz.id_db");
            // CheckContent(cntl, "rpcz.time_db");
        }
        
        StopAndJoin();
    }
    
private:
    melon::Server _server;
};

TEST_F(BuiltinServiceTest, index) {
    TestIndex(false);
    TestIndex(true);
}

TEST_F(BuiltinServiceTest, version) {
    const std::string VERSION = "test_version";
    melon::VersionService service(&_server);
    melon::VersionRequest req;
    melon::VersionResponse res;
    melon::Controller cntl;
    ClosureChecker done;
    _server.set_version(VERSION);
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    EXPECT_EQ(VERSION, cntl.response_attachment().to_string());
}

TEST_F(BuiltinServiceTest, health) {
    const std::string HEALTH_STR = "OK";
    melon::HealthService service;
    melon::HealthRequest req;
    melon::HealthResponse res;
    melon::Controller cntl;
    SetUpController(&cntl, false);
    ClosureChecker done;
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    EXPECT_EQ(HEALTH_STR, cntl.response_attachment().to_string());
}

class MyHealthReporter : public melon::HealthReporter {
public:
    void GenerateReport(melon::Controller* cntl,
                        google::protobuf::Closure* done) {
        cntl->response_attachment().append("i'm ok");
        done->Run();
    }
};

TEST_F(BuiltinServiceTest, customized_health) {
    melon::ServerOptions opt;
    MyHealthReporter hr;
    opt.health_reporter = &hr;
    ASSERT_EQ(0, _server.Start(9798, &opt));
    melon::HealthRequest req;
    melon::HealthResponse res;
    melon::ChannelOptions copt;
    copt.protocol = melon::PROTOCOL_HTTP;
    melon::Channel chan;
    ASSERT_EQ(0, chan.Init("127.0.0.1:9798", &copt));
    melon::Controller cntl;
    cntl.http_request().uri() = "/health";
    chan.CallMethod(nullptr, &cntl, &req, &res, nullptr);
    EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
    EXPECT_EQ("i'm ok", cntl.response_attachment());
}

class MyGrpcHealthReporter : public melon::HealthReporter {
public:
    void GenerateReport(melon::Controller* cntl,
                        google::protobuf::Closure* done) {
        grpc::health::v1::HealthCheckResponse response;
        response.set_status(grpc::health::v1::HealthCheckResponse_ServingStatus_UNKNOWN);

        if (cntl->response()) {
            cntl->response()->CopyFrom(response);
        } else {
            std::string json;
            json2pb::ProtoMessageToJson(response, &json);
            cntl->http_response().set_content_type("application/json");
            cntl->response_attachment().append(json);
        }
        done->Run();
    }
};

TEST_F(BuiltinServiceTest, normal_grpc_health) {
    melon::ServerOptions opt;
    ASSERT_EQ(0, _server.Start(9798, &opt));

    grpc::health::v1::HealthCheckResponse response;
    grpc::health::v1::HealthCheckRequest request;
    request.set_service("grpc_req_from_rpc");
    melon::Controller cntl;
    melon::ChannelOptions copt;
    copt.protocol = "h2:grpc";
    melon::Channel chan;
    ASSERT_EQ(0, chan.Init("127.0.0.1:9798", &copt));
    grpc::health::v1::Health_Stub stub(&chan);
    stub.Check(&cntl, &request, &response, nullptr);
    EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
    EXPECT_EQ(response.status(), grpc::health::v1::HealthCheckResponse_ServingStatus_SERVING);

    response.Clear();
    melon::Controller cntl1;
    cntl1.http_request().uri() = "/grpc.health.v1.Health/Check";
    chan.CallMethod(nullptr, &cntl1, &request, &response, nullptr);
    EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
    EXPECT_EQ(response.status(), grpc::health::v1::HealthCheckResponse_ServingStatus_SERVING);
}

TEST_F(BuiltinServiceTest, customized_grpc_health) {
    melon::ServerOptions opt;
    MyGrpcHealthReporter hr;
    opt.health_reporter = &hr;
    ASSERT_EQ(0, _server.Start(9798, &opt));

    grpc::health::v1::HealthCheckResponse response;
    grpc::health::v1::HealthCheckRequest request;
    request.set_service("grpc_req_from_rpc");
    melon::Controller cntl;

    melon::ChannelOptions copt;
    copt.protocol = "h2:grpc";
    melon::Channel chan;
    ASSERT_EQ(0, chan.Init("127.0.0.1:9798", &copt));

    grpc::health::v1::Health_Stub stub(&chan);
    stub.Check(&cntl, &request, &response, nullptr);

    EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
    EXPECT_EQ(response.status(), grpc::health::v1::HealthCheckResponse_ServingStatus_UNKNOWN);
}

TEST_F(BuiltinServiceTest, status) {
    TestStatus(false);
    TestStatus(true);
}

TEST_F(BuiltinServiceTest, list) {
    melon::ListService service(&_server);
    melon::ListRequest req;
    melon::ListResponse res;
    melon::Controller cntl;
    ClosureChecker done;
    ASSERT_EQ(0, _server.AddService(new EchoServiceImpl(),
                                    melon::SERVER_OWNS_SERVICE));
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    ASSERT_EQ(1, res.service_size());
    EXPECT_EQ(test::EchoService::descriptor()->name(), res.service(0).name());
}

void* sleep_thread(void*) {
    sleep(1);
    return nullptr;
}

TEST_F(BuiltinServiceTest, threads) {
    melon::ThreadsService service;
    melon::ThreadsRequest req;
    melon::ThreadsResponse res;
    melon::Controller cntl;
    ClosureChecker done;
    pthread_t tid;
    ASSERT_EQ(0, pthread_create(&tid, nullptr, sleep_thread, nullptr));
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    // Doesn't work under gcc 4.8.2
    // CheckContent(cntl, "sleep_thread");
    pthread_join(tid, nullptr);
}

TEST_F(BuiltinServiceTest, vlog) {
    TestVLog(false);
    TestVLog(true);
}

TEST_F(BuiltinServiceTest, connections) {
    TestConnections(false);
    TestConnections(true);
}

TEST_F(BuiltinServiceTest, flags) {
    TestFlags(false);
    TestFlags(true);
}

TEST_F(BuiltinServiceTest, bad_method) {
    TestBadMethod(false);
    TestBadMethod(true);
}

TEST_F(BuiltinServiceTest, vars) {
    // Start server to show bvars inside 
    ASSERT_EQ(0, _server.Start("127.0.0.1:9798", nullptr));
    melon::VarsService service;
    melon::VarsRequest req;
    melon::VarsResponse res;
    {
        ClosureChecker done;
        melon::Controller cntl;
        melon::var::Adder<int64_t> myvar;
        myvar.expose("myvar");
        myvar << 9;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckFieldInContent(cntl, "myvar : ", 9);
    }
    {
        ClosureChecker done;
        melon::Controller cntl;
        cntl.http_request()._unresolved_path = "iobuf*";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "iobuf_block_count");
    }
}

TEST_F(BuiltinServiceTest, rpcz) {
    for (int i = 0; i <= 1; ++i) {  // enable rpcz
        for (int j = 0; j <= 1; ++j) {  // hex log id
            for (int k = 0; k <= 1; ++k) {  // use html
                TestRpcz(i, j, k);
            }
        }
    }
}

TEST_F(BuiltinServiceTest, pprof) {
    melon::PProfService service;
    {
        ClosureChecker done;
        melon::Controller cntl;
        cntl.http_request().uri().SetQuery("seconds", "1");
        service.profile(&cntl, nullptr, nullptr, &done);
        // Just for loading symbols in gperftools/profiler.h
        ProfilerFlush();
        EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
        EXPECT_GT(cntl.response_attachment().length(), 0ul);
    }
    {
        ClosureChecker done;
        melon::Controller cntl;
        service.heap(&cntl, nullptr, nullptr, &done);
        const int rc = getenv("TCMALLOC_SAMPLE_PARAMETER") != nullptr ? 0 : melon::ENOMETHOD;
        EXPECT_EQ(rc, cntl.ErrorCode()) << cntl.ErrorText();
    }
    {
        ClosureChecker done;
        melon::Controller cntl;
        service.growth(&cntl, nullptr, nullptr, &done);
        // linked tcmalloc in UT
        EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
    }
    {
        ClosureChecker done;
        melon::Controller cntl;
        service.symbol(&cntl, nullptr, nullptr, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "num_symbols");
    }
    {
        ClosureChecker done;
        melon::Controller cntl;
        service.cmdline(&cntl, nullptr, nullptr, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "rpc_builtin_service_unittest");
    }
}

TEST_F(BuiltinServiceTest, dir) {
    melon::DirService service;
    melon::DirRequest req;
    melon::DirResponse res;
    {
        // Open root path
        ClosureChecker done;
        melon::Controller cntl;
        SetUpController(&cntl, true);
        cntl.http_request()._unresolved_path = "";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "tmp");
    }
    {
        // Open a specific file
        ClosureChecker done;
        melon::Controller cntl;
        SetUpController(&cntl, false);
        cntl.http_request()._unresolved_path = "/usr/include/errno.h";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
#if defined(OS_LINUX)
        CheckContent(cntl, "ERRNO_H");
#elif defined(OS_MACOSX)
        CheckContent(cntl, "sys/errno.h");
#endif
    }
    {
        // Open a file that doesn't exist
        ClosureChecker done;
        melon::Controller cntl;
        SetUpController(&cntl, false);
        cntl.http_request()._unresolved_path = "file_not_exist";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_TRUE(cntl.Failed());
        CheckErrorText(cntl, "Cannot open");
    }
}
    
TEST_F(BuiltinServiceTest, ids) {
    melon::IdsService service;
    melon::IdsRequest req;
    melon::IdsResponse res;
    {
        ClosureChecker done;
        melon::Controller cntl;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "Use /ids/<call_id>");
    }    
    {
        ClosureChecker done;
        melon::Controller cntl;
        cntl.http_request()._unresolved_path = "not_valid";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_TRUE(cntl.Failed());
        CheckErrorText(cntl, "is not a fiber_session");
    }    
    {
        fiber_session_t id;
        EXPECT_EQ(0, fiber_session_create(&id, nullptr, nullptr));
        ClosureChecker done;
        melon::Controller cntl;
        std::string id_string;
        mutil::string_printf(&id_string, "%llu", (unsigned long long)id.value);
        cntl.http_request()._unresolved_path = id_string;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "Status: UNLOCKED");
    }    
}

void* dummy_fiber(void*) {
    fiber_usleep(1000000);
    return nullptr;
}

TEST_F(BuiltinServiceTest, fibers) {
    melon::FibersService service;
    melon::FibersRequest req;
    melon::FibersResponse res;
    {
        ClosureChecker done;
        melon::Controller cntl;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "Use /fibers/<fiber_session>");
    }    
    {
        ClosureChecker done;
        melon::Controller cntl;
        cntl.http_request()._unresolved_path = "not_valid";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_TRUE(cntl.Failed());
        CheckErrorText(cntl, "is not a fiber id");
    }    
    {
        fiber_t th;
        EXPECT_EQ(0, fiber_start_background(&th, nullptr, dummy_fiber, nullptr));
        ClosureChecker done;
        melon::Controller cntl;
        std::string id_string;
        mutil::string_printf(&id_string, "%llu", (unsigned long long)th);
        cntl.http_request()._unresolved_path = id_string;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "stop=0");
    }    
}

TEST_F(BuiltinServiceTest, sockets) {
    melon::SocketsService service;
    melon::SocketsRequest req;
    melon::SocketsResponse res;
    {
        ClosureChecker done;
        melon::Controller cntl;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "Use /sockets/<SocketId>");
    }    
    {
        ClosureChecker done;
        melon::Controller cntl;
        cntl.http_request()._unresolved_path = "not_valid";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_TRUE(cntl.Failed());
        CheckErrorText(cntl, "is not a SocketId");
    }    
    {
        melon::SocketId id;
        melon::SocketOptions options;
        EXPECT_EQ(0, melon::Socket::Create(options, &id));
        ClosureChecker done;
        melon::Controller cntl;
        std::string id_string;
        mutil::string_printf(&id_string, "%llu", (unsigned long long)id);
        cntl.http_request()._unresolved_path = id_string;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "fd=-1");
    }    
}

TEST_F(BuiltinServiceTest, memory) {
    melon::MemoryService service;
    melon::MemoryRequest req;
    melon::MemoryResponse res;
    melon::Controller cntl;
    ClosureChecker done;
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    CheckContent(cntl, "generic.current_allocated_bytes");
    CheckContent(cntl, "generic.heap_size");
    CheckContent(cntl, "tcmalloc.current_total_thread_cache_bytes");
    CheckContent(cntl, "tcmalloc.central_cache_free_bytes");
    CheckContent(cntl, "tcmalloc.transfer_cache_free_bytes");
    CheckContent(cntl, "tcmalloc.thread_cache_free_bytes");
    CheckContent(cntl, "tcmalloc.pageheap_free_bytes");
    CheckContent(cntl, "tcmalloc.pageheap_unmapped_bytes");
}
