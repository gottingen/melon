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
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fstream>
#include "testing/gtest_wrap.h"
#include <gflags/gflags.h>
#include <google/protobuf/descriptor.h>
#include "melon/base/gperftools_profiler.h"
#include "melon/times/time.h"
#include "melon/rpc/socket.h"
#include "melon/rpc/server.h"
#include "melon/rpc/channel.h"
#include "melon/rpc/controller.h"
#include "melon/rpc/span.h"
#include "melon/rpc/reloadable_flags.h"
#include "melon/rpc/builtin/version_service.h"
#include "melon/rpc/builtin/health_service.h"
#include "melon/rpc/builtin/list_service.h"
#include "melon/rpc/builtin/status_service.h"
#include "melon/rpc/builtin/threads_service.h"
#include "melon/rpc/builtin/index_service.h"        // IndexService
#include "melon/rpc/builtin/connections_service.h"  // ConnectionsService
#include "melon/rpc/builtin/flags_service.h"        // FlagsService
#include "melon/rpc/builtin/vars_service.h"         // VarsService
#include "melon/rpc/builtin/rpcz_service.h"         // RpczService
#include "melon/rpc/builtin/dir_service.h"          // DirService
#include "melon/rpc/builtin/pprof_service.h"        // PProfService
#include "melon/rpc/builtin/fibers_service.h"     // FibersService
#include "melon/rpc/builtin/token_service.h"          // TokenService
#include "melon/rpc/builtin/sockets_service.h"      // SocketsService
#include "melon/rpc/builtin/common.h"
#include "melon/rpc/builtin/bad_method_service.h"
#include "echo.pb.h"

DEFINE_bool(foo, false, "Flags for UT");
MELON_RPC_VALIDATE_GFLAG(foo, melon::rpc::PassValidate);

namespace melon::rpc {
    DECLARE_bool(enable_rpcz);
    DECLARE_bool(rpcz_hex_log_id);
    DECLARE_int32(idle_timeout_second);
} // namespace rpc

int main(int argc, char *argv[]) {
    melon::rpc::FLAGS_idle_timeout_second = 0;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class EchoServiceImpl : public test::EchoService {
public:
    EchoServiceImpl() {}

    virtual ~EchoServiceImpl() {}

    virtual void Echo(google::protobuf::RpcController *cntl_base,
                      const test::EchoRequest *req,
                      test::EchoResponse *res,
                      google::protobuf::Closure *done) {
        melon::rpc::Controller *cntl =
                static_cast<melon::rpc::Controller *>(cntl_base);
        melon::rpc::ClosureGuard done_guard(done);
        TRACEPRINTF("MyAnnotation: %ld", cntl->log_id());
        if (req->sleep_us() > 0) {
            melon::fiber_sleep_for(req->sleep_us());
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
    MELON_VLOG(3) << "This is a MELON_VLOG!";
}

void CheckContent(const melon::rpc::Controller &cntl, const char *name) {
    const std::string &content = cntl.response_attachment().to_string();
    std::size_t pos = content.find(name);
    ASSERT_TRUE(pos != std::string::npos) << "name=" << name<<"\n content="<<content;
}

void CheckErrorText(const melon::rpc::Controller &cntl, const char *error) {
    std::size_t pos = cntl.ErrorText().find(error);
    ASSERT_TRUE(pos != std::string::npos) << "error=" << error;
}

void CheckFieldInContent(const melon::rpc::Controller &cntl,
                         const char *name, int32_t expect) {
    const std::string &content = cntl.response_attachment().to_string();
    std::size_t pos = content.find(name);
    ASSERT_TRUE(pos != std::string::npos);

    int32_t val = 0;
    ASSERT_EQ(1, sscanf(content.c_str() + pos + strlen(name), "%d", &val));
    ASSERT_EQ(expect, val) << "name=" << name;
}

void CheckAnnotation(const melon::rpc::Controller &cntl, int64_t expect) {
    const std::string &content = cntl.response_attachment().to_string();
    std::string expect_str;
    melon::string_printf(&expect_str, "MyAnnotation: %" PRId64, expect);
    std::size_t pos = content.find(expect_str);
    ASSERT_TRUE(pos != std::string::npos) << expect;
}

void CheckTraceId(const melon::rpc::Controller &cntl,
                  const std::string &expect_id_str) {
    const std::string &content = cntl.response_attachment().to_string();
    std::string expect_str = std::string(melon::rpc::TRACE_ID_STR) + "=" + expect_id_str;
    std::size_t pos = content.find(expect_str);
    ASSERT_TRUE(pos != std::string::npos) << expect_str;
}

class BuiltinServiceTest : public ::testing::Test {
protected:

    BuiltinServiceTest() {};

    virtual ~BuiltinServiceTest() {};

    virtual void SetUp() { EXPECT_EQ(0, _server.AddBuiltinServices()); }

    virtual void TearDown() { StopAndJoin(); }

    void StopAndJoin() {
        _server.Stop(0);
        _server.Join();
        _server.ClearServices();
    }

    void SetUpController(melon::rpc::Controller *cntl, bool use_html) const {
        cntl->_server = &_server;
        if (use_html) {
            cntl->http_request().SetHeader(
                    melon::rpc::USER_AGENT_STR, "just keep user agent non-empty");
        }
    }

    void TestIndex(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        melon::rpc::IndexService service;
        melon::rpc::IndexRequest req;
        melon::rpc::IndexResponse res;
        melon::rpc::Controller cntl;
        ClosureChecker done;
        SetUpController(&cntl, use_html);
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        EXPECT_EQ(expect_type, cntl.http_response().content_type());
    }

    void TestStatus(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        melon::rpc::StatusService service;
        melon::rpc::StatusRequest req;
        melon::rpc::StatusResponse res;
        melon::rpc::Controller cntl;
        ClosureChecker done;
        SetUpController(&cntl, use_html);
        EchoServiceImpl echo_svc;
        ASSERT_EQ(0, _server.AddService(
                &echo_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        EXPECT_EQ(expect_type, cntl.http_response().content_type());
        ASSERT_EQ(0, _server.RemoveService(&echo_svc));
    }


    void TestConnections(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        melon::rpc::ConnectionsService service;
        melon::rpc::ConnectionsRequest req;
        melon::rpc::ConnectionsResponse res;
        melon::rpc::Controller cntl;
        ClosureChecker done;
        SetUpController(&cntl, use_html);
        melon::end_point ep;
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
        melon::rpc::BadMethodService service;
        melon::rpc::BadMethodResponse res;
        {
            ClosureChecker done;
            melon::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            melon::rpc::BadMethodRequest req;
            req.set_service_name(
                    melon::rpc::PProfService::descriptor()->full_name());
            service.no_method(&cntl, &req, &res, &done);
            EXPECT_EQ(melon::rpc::ENOMETHOD, cntl.ErrorCode());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            CheckErrorText(cntl, "growth");
        }
    }

    void TestFlags(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        melon::rpc::FlagsService service;
        melon::rpc::FlagsRequest req;
        melon::rpc::FlagsResponse res;
        {
            ClosureChecker done;
            melon::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            CheckContent(cntl, "fiber_concurrency");
        }
        {
            ClosureChecker done;
            melon::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request()._unresolved_path = "foo";
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            CheckContent(cntl, "false");
        }
        {
            ClosureChecker done;
            melon::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request()._unresolved_path = "foo";
            cntl.http_request().uri()
                    .SetQuery(melon::rpc::SETVALUE_STR, "true");
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
        }
        {
            ClosureChecker done;
            melon::rpc::Controller cntl;
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
        melon::rpc::RpczService service;
        melon::rpc::RpczRequest req;
        melon::rpc::RpczResponse res;
        if (!enable) {
            {
                ClosureChecker done;
                melon::rpc::Controller cntl;
                SetUpController(&cntl, use_html);
                service.disable(&cntl, &req, &res, &done);
                EXPECT_FALSE(cntl.Failed());
                EXPECT_FALSE(melon::rpc::FLAGS_enable_rpcz);
            }
            {
                ClosureChecker done;
                melon::rpc::Controller cntl;
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
                melon::rpc::Controller cntl;
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
            melon::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            service.enable(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            EXPECT_TRUE(melon::rpc::FLAGS_enable_rpcz);
        }

        if (hex) {
            ClosureChecker done;
            melon::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            service.hex_log_id(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_TRUE(melon::rpc::FLAGS_rpcz_hex_log_id);
        } else {
            ClosureChecker done;
            melon::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            service.dec_log_id(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_FALSE(melon::rpc::FLAGS_rpcz_hex_log_id);
        }

        ASSERT_EQ(0, _server.AddService(new EchoServiceImpl(),
                                        melon::rpc::SERVER_OWNS_SERVICE));
        melon::end_point ep;
        ASSERT_EQ(0, str2endpoint("127.0.0.1:9748", &ep));
        ASSERT_EQ(0, _server.Start(ep, nullptr));
        melon::rpc::Channel channel;
        ASSERT_EQ(0, channel.Init(ep, nullptr));
        test::EchoService_Stub stub(&channel);
        int64_t log_id = 1234567890;
        char querystr_buf[128];
        // Since LevelDB is unstable on jerkins, disable all the assertions here
        {
            // Find by trace_id
            test::EchoRequest echo_req;
            test::EchoResponse echo_res;
            melon::rpc::Controller echo_cntl;
            echo_req.set_message("hello");
            echo_cntl.set_log_id(++log_id);
            stub.Echo(&echo_cntl, &echo_req, &echo_res, nullptr);
            EXPECT_FALSE(echo_cntl.Failed());

            // Wait for level db to commit span information
            usleep(500000);
            ClosureChecker done;
            melon::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request().uri()
                    .SetQuery(melon::rpc::TRACE_ID_STR, echo_res.message());
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            // CheckAnnotation(cntl, log_id);
        }

        {
            // Find by latency
            test::EchoRequest echo_req;
            test::EchoResponse echo_res;
            melon::rpc::Controller echo_cntl;
            echo_req.set_message("hello");
            echo_req.set_sleep_us(150000);
            echo_cntl.set_log_id(++log_id);
            stub.Echo(&echo_cntl, &echo_req, &echo_res, nullptr);
            EXPECT_FALSE(echo_cntl.Failed());

            // Wait for level db to commit span information
            usleep(500000);
            ClosureChecker done;
            melon::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request().uri()
                    .SetQuery(melon::rpc::MIN_LATENCY_STR, "100000");
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            // CheckTraceId(cntl, echo_res.message());
        }

        {
            // Find by request size
            test::EchoRequest echo_req;
            test::EchoResponse echo_res;
            melon::rpc::Controller echo_cntl;
            std::string request_str(1500, 'a');
            echo_req.set_message(request_str);
            echo_cntl.set_log_id(++log_id);
            stub.Echo(&echo_cntl, &echo_req, &echo_res, nullptr);
            EXPECT_FALSE(echo_cntl.Failed());

            // Wait for level db to commit span information
            usleep(500000);
            ClosureChecker done;
            melon::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request().uri()
                    .SetQuery(melon::rpc::MIN_REQUEST_SIZE_STR, "1024");
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            // CheckTraceId(cntl, echo_res.message());
        }

        {
            // Find by log id
            test::EchoRequest echo_req;
            test::EchoResponse echo_res;
            melon::rpc::Controller echo_cntl;
            echo_req.set_message("hello");
            echo_cntl.set_log_id(++log_id);
            stub.Echo(&echo_cntl, &echo_req, &echo_res, nullptr);
            EXPECT_FALSE(echo_cntl.Failed());

            // Wait for level db to commit span information
            usleep(500000);
            ClosureChecker done;
            melon::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            snprintf(querystr_buf, sizeof(querystr_buf), "%" PRId64, log_id);
            cntl.http_request().uri()
                    .SetQuery(melon::rpc::LOG_ID_STR, querystr_buf);
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            // CheckTraceId(cntl, echo_res.message());
        }

        {
            ClosureChecker done;
            melon::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            service.stats(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            // CheckContent(cntl, "rpcz.id_db");
            // CheckContent(cntl, "rpcz.time_db");
        }

        StopAndJoin();
    }

private:
    melon::rpc::Server _server;
};

TEST_F(BuiltinServiceTest, index) {
    TestIndex(false);
    TestIndex(true);
}

TEST_F(BuiltinServiceTest, version) {
    const std::string VERSION = "test_version";
    melon::rpc::VersionService service(&_server);
    melon::rpc::VersionRequest req;
    melon::rpc::VersionResponse res;
    melon::rpc::Controller cntl;
    ClosureChecker done;
    _server.set_version(VERSION);
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    EXPECT_EQ(VERSION, cntl.response_attachment().to_string());
}

TEST_F(BuiltinServiceTest, health) {
    const std::string HEALTH_STR = "OK";
    melon::rpc::HealthService service;
    melon::rpc::HealthRequest req;
    melon::rpc::HealthResponse res;
    melon::rpc::Controller cntl;
    SetUpController(&cntl, false);
    ClosureChecker done;
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    EXPECT_EQ(HEALTH_STR, cntl.response_attachment().to_string());
}

class MyHealthReporter : public melon::rpc::HealthReporter {
public:
    void GenerateReport(melon::rpc::Controller *cntl,
                        google::protobuf::Closure *done) {
        cntl->response_attachment().append("i'm ok");
        done->Run();
    }
};

TEST_F(BuiltinServiceTest, customized_health) {
    melon::rpc::ServerOptions opt;
    MyHealthReporter hr;
    opt.health_reporter = &hr;
    ASSERT_EQ(0, _server.Start(9798, &opt));
    melon::rpc::HealthRequest req;
    melon::rpc::HealthResponse res;
    melon::rpc::ChannelOptions copt;
    copt.protocol = melon::rpc::PROTOCOL_HTTP;
    melon::rpc::Channel chan;
    ASSERT_EQ(0, chan.Init("127.0.0.1:9798", &copt));
    melon::rpc::Controller cntl;
    cntl.http_request().uri() = "/health";
    chan.CallMethod(nullptr, &cntl, &req, &res, nullptr);
    EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
    EXPECT_EQ("i'm ok", cntl.response_attachment());
}

TEST_F(BuiltinServiceTest, status) {
    TestStatus(false);
    TestStatus(true);
}

TEST_F(BuiltinServiceTest, list) {
    melon::rpc::ListService service(&_server);
    melon::rpc::ListRequest req;
    melon::rpc::ListResponse res;
    melon::rpc::Controller cntl;
    ClosureChecker done;
    ASSERT_EQ(0, _server.AddService(new EchoServiceImpl(),
                                    melon::rpc::SERVER_OWNS_SERVICE));
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    ASSERT_EQ(1, res.service_size());
    EXPECT_EQ(test::EchoService::descriptor()->name(), res.service(0).name());
}

void *sleep_thread(void *) {
    sleep(1);
    return nullptr;
}

TEST_F(BuiltinServiceTest, threads) {
    melon::rpc::ThreadsService service;
    melon::rpc::ThreadsRequest req;
    melon::rpc::ThreadsResponse res;
    melon::rpc::Controller cntl;
    ClosureChecker done;
    pthread_t tid;
    ASSERT_EQ(0, pthread_create(&tid, nullptr, sleep_thread, nullptr));
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    // Doesn't work under gcc 4.8.2
    // CheckContent(cntl, "sleep_thread");
    pthread_join(tid, nullptr);
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
    // Start server to show variables inside
    ASSERT_EQ(0, _server.Start("127.0.0.1:9798", nullptr));
    melon::rpc::VarsService service;
    melon::rpc::VarsRequest req;
    melon::rpc::VarsResponse res;
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
        melon::gauge<int64_t> myvar;
        myvar.expose("myvar", "");
        myvar << 9;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckFieldInContent(cntl, "myvar : ", 9);
    }
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
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
    melon::rpc::PProfService service;
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
        cntl.http_request().uri().SetQuery("seconds", "1");
        service.profile(&cntl, nullptr, nullptr, &done);
        // Just for loading symbols in gperftools/profiler.h
        ProfilerFlush();
        EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
        EXPECT_GT(cntl.response_attachment().length(), 0ul);
    }
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
        service.heap(&cntl, nullptr, nullptr, &done);
        const int rc = getenv("TCMALLOC_SAMPLE_PARAMETER") != nullptr ? 0 : melon::rpc::ENOMETHOD;
        EXPECT_EQ(rc, cntl.ErrorCode()) << cntl.ErrorText();
    }
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
        service.growth(&cntl, nullptr, nullptr, &done);
        // linked tcmalloc in UT
        EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
    }
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
        service.symbol(&cntl, nullptr, nullptr, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "num_symbols");
    }
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
        service.cmdline(&cntl, nullptr, nullptr, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "rpc_builtin_service_test");
    }
}

TEST_F(BuiltinServiceTest, dir) {
    melon::rpc::DirService service;
    melon::rpc::DirRequest req;
    melon::rpc::DirResponse res;
    {
        // Open root path
        ClosureChecker done;
        melon::rpc::Controller cntl;
        SetUpController(&cntl, true);
        cntl.http_request()._unresolved_path = "";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "tmp");
    }
    {
        // Open a specific file
        ClosureChecker done;
        melon::rpc::Controller cntl;
        SetUpController(&cntl, false);
        cntl.http_request()._unresolved_path = "/usr/include/errno.h";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
#if defined(MELON_PLATFORM_LINUX)
        CheckContent(cntl, "ERRNO_H");
#elif defined(MELON_PLATFORM_OSX)
        CheckContent(cntl, "sys/errno.h");
#endif
    }
    {
        // Open a file that doesn't exist
        ClosureChecker done;
        melon::rpc::Controller cntl;
        SetUpController(&cntl, false);
        cntl.http_request()._unresolved_path = "file_not_exist";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_TRUE(cntl.Failed());
        CheckErrorText(cntl, "Cannot open");
    }
}

TEST_F(BuiltinServiceTest, token) {
    melon::rpc::TokenService service;
    melon::rpc::TokenRequest req;
    melon::rpc::TokenResponse res;
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "Use /token/<call_id>");
    }
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
        cntl.http_request()._unresolved_path = "not_valid";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_TRUE(cntl.Failed());
        CheckErrorText(cntl, "is not a fiber_token");
    }
    {
        fiber_token_t id;
        EXPECT_EQ(0, fiber_token_create(&id, nullptr, nullptr));
        ClosureChecker done;
        melon::rpc::Controller cntl;
        std::string id_string;
        melon::string_printf(&id_string, "%llu", (unsigned long long) id.value);
        cntl.http_request()._unresolved_path = id_string;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "Status: UNLOCKED");
    }
}

void *dummy_fiber(void *) {
    melon::fiber_sleep_for(1000000);
    return nullptr;
}

TEST_F(BuiltinServiceTest, fibers) {
    melon::rpc::FibersService service;
    melon::rpc::FibersRequest req;
    melon::rpc::FibersResponse res;
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "Use /fibers/<fiber_id>");
    }
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
        cntl.http_request()._unresolved_path = "not_valid";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_TRUE(cntl.Failed());
        CheckErrorText(cntl, "is not a fiber id");
    }
    {
        fiber_id_t th;
        EXPECT_EQ(0, fiber_start_background(&th, nullptr, dummy_fiber, nullptr));
        ClosureChecker done;
        melon::rpc::Controller cntl;
        std::string id_string;
        melon::string_printf(&id_string, "%llu", (unsigned long long) th);
        cntl.http_request()._unresolved_path = id_string;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "stop=0");
    }
}

TEST_F(BuiltinServiceTest, sockets) {
    melon::rpc::SocketsService service;
    melon::rpc::SocketsRequest req;
    melon::rpc::SocketsResponse res;
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "Use /sockets/<SocketId>");
    }
    {
        ClosureChecker done;
        melon::rpc::Controller cntl;
        cntl.http_request()._unresolved_path = "not_valid";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_TRUE(cntl.Failed());
        CheckErrorText(cntl, "is not a SocketId");
    }
    {
        melon::rpc::SocketId id;
        melon::rpc::SocketOptions options;
        EXPECT_EQ(0, melon::rpc::Socket::Create(options, &id));
        ClosureChecker done;
        melon::rpc::Controller cntl;
        std::string id_string;
        melon::string_printf(&id_string, "%llu", (unsigned long long) id);
        cntl.http_request()._unresolved_path = id_string;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "fd=-1");
    }
}
