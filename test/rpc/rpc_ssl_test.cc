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

// Baidu RPC - A framework to host and access services throughout Baidu.

// Date: Sun Jul 13 15:04:18 CST 2014

#include <fstream>
#include "testing/gtest_wrap.h"
#include <google/protobuf/descriptor.h>
#include "melon/times/time.h"
#include "melon/base/fd_guard.h"
#include <melon/files/sequential_read_file.h>
#include "melon/rpc/global.h"
#include "melon/rpc/socket.h"
#include "melon/rpc/server.h"
#include "melon/rpc/channel.h"
#include "melon/rpc/socket_map.h"
#include "melon/rpc/controller.h"
#include "echo.pb.h"

namespace melon::rpc {
    void ExtractHostnames(X509 *x, std::vector<std::string> *hostnames);
} // namespace melon::rpc


int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    melon::rpc::GlobalInitializeOrDie();
    return RUN_ALL_TESTS();
}

bool g_delete = false;
const std::string EXP_REQUEST = "hello";
const std::string EXP_RESPONSE = "world";

class EchoServiceImpl : public test::EchoService {
public:
    EchoServiceImpl() : count(0) {}

    virtual ~EchoServiceImpl() { g_delete = true; }

    virtual void Echo(google::protobuf::RpcController *cntl_base,
                      const test::EchoRequest *request,
                      test::EchoResponse *response,
                      google::protobuf::Closure *done) {
        melon::rpc::ClosureGuard done_guard(done);
        melon::rpc::Controller *cntl = (melon::rpc::Controller *) cntl_base;
        count.fetch_add(1, std::memory_order_relaxed);
        EXPECT_EQ(EXP_REQUEST, request->message());
        EXPECT_TRUE(cntl->is_ssl());

        response->set_message(EXP_RESPONSE);
        if (request->sleep_us() > 0) {
            MELON_LOG(INFO) << "Sleep " << request->sleep_us() << " us, protocol="
                      << cntl->request_protocol();
            melon::fiber_sleep_for(request->sleep_us());
        }
    }

    std::atomic<int64_t> count;
};

class SSLTest : public ::testing::Test {
protected:
    SSLTest() {};

    virtual ~SSLTest() {};

    virtual void SetUp() {};

    virtual void TearDown() {};
};

void *RunClosure(void *arg) {
    google::protobuf::Closure *done = (google::protobuf::Closure *) arg;
    done->Run();
    return nullptr;
}

void SendMultipleRPC(melon::rpc::Channel *channel, int count) {
    for (int i = 0; i < count; ++i) {
        melon::rpc::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(EXP_REQUEST);
        test::EchoService_Stub stub(channel);
        stub.Echo(&cntl, &req, &res, nullptr);

        EXPECT_EQ(EXP_RESPONSE, res.message()) << cntl.ErrorText();
    }
}

TEST_F(SSLTest, sanity) {
    // Test RPC based on SSL + melon protocol
    const int port = 8613;
    melon::rpc::Server server;
    melon::rpc::ServerOptions options;

    melon::rpc::CertInfo cert;
    cert.certificate = "cert1.crt";
    cert.private_key = "cert1.key";
    options.mutable_ssl_options()->default_cert = cert;

    EchoServiceImpl echo_svc;
    ASSERT_EQ(0, server.AddService(
            &echo_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(port, &options));

    test::EchoRequest req;
    test::EchoResponse res;
    req.set_message(EXP_REQUEST);
    {
        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions coptions;
        coptions.mutable_ssl_options();
        coptions.mutable_ssl_options()->sni_name = "localhost";
        ASSERT_EQ(0, channel.Init("localhost", port, &coptions));

        melon::rpc::Controller cntl;
        test::EchoService_Stub stub(&channel);
        stub.Echo(&cntl, &req, &res, nullptr);
        EXPECT_EQ(EXP_RESPONSE, res.message()) << cntl.ErrorText();
    }

    // stress test
    const int NUM = 5;
    const int COUNT = 3000;
    pthread_t tids[NUM];
    {
        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions coptions;
        coptions.mutable_ssl_options();
        coptions.mutable_ssl_options()->sni_name = "localhost";
        ASSERT_EQ(0, channel.Init("127.0.0.1", port, &coptions));
        for (int i = 0; i < NUM; ++i) {
            google::protobuf::Closure *thrd_func =
                    melon::rpc::NewCallback(SendMultipleRPC, &channel, COUNT);
            EXPECT_EQ(0, pthread_create(&tids[i], nullptr, RunClosure, thrd_func));
        }
        for (int i = 0; i < NUM; ++i) {
            pthread_join(tids[i], nullptr);
        }
    }
    {
        // Use HTTP
        melon::rpc::Channel channel;
        melon::rpc::ChannelOptions coptions;
        coptions.protocol = "http";
        coptions.mutable_ssl_options();
        coptions.mutable_ssl_options()->sni_name = "localhost";
        ASSERT_EQ(0, channel.Init("127.0.0.1", port, &coptions));
        for (int i = 0; i < NUM; ++i) {
            google::protobuf::Closure *thrd_func =
                    melon::rpc::NewCallback(SendMultipleRPC, &channel, COUNT);
            EXPECT_EQ(0, pthread_create(&tids[i], nullptr, RunClosure, thrd_func));
        }
        for (int i = 0; i < NUM; ++i) {
            pthread_join(tids[i], nullptr);
        }
    }

    ASSERT_EQ(0, server.Stop(0));
    ASSERT_EQ(0, server.Join());
}

void CheckCert(const char *cname, const char *cert) {
    const int port = 8613;
    melon::rpc::Channel channel;
    melon::rpc::ChannelOptions coptions;
    coptions.mutable_ssl_options()->sni_name = cname;
    ASSERT_EQ(0, channel.Init("127.0.0.1", port, &coptions));

    SendMultipleRPC(&channel, 1);
    // client has no access to the sending socket
    std::vector<melon::rpc::SocketId> ids;
    melon::rpc::SocketMapList(&ids);
    ASSERT_EQ(1u, ids.size());
    melon::rpc::SocketUniquePtr sock;
    ASSERT_EQ(0, melon::rpc::Socket::Address(ids[0], &sock));

    X509 *x509 = sock->GetPeerCertificate();
    ASSERT_TRUE(x509 != nullptr);
    std::vector<std::string> cnames;
    melon::rpc::ExtractHostnames(x509, &cnames);
    ASSERT_EQ(cert, cnames[0]) << x509;
}

std::string GetRawPemString(const char *fname) {
    melon::sequential_read_file file;
    file.open(fname);
    std::string content;
    auto fs = file.read(&content);
    return content;
}

#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME

TEST_F(SSLTest, ssl_sni) {
    const int port = 8613;
    melon::rpc::Server server;
    melon::rpc::ServerOptions options;
    {
        melon::rpc::CertInfo cert;
        cert.certificate = "cert1.crt";
        cert.private_key = "cert1.key";
        cert.sni_filters.push_back("cert1.com");
        options.mutable_ssl_options()->default_cert = cert;
    }
    {
        melon::rpc::CertInfo cert;
        cert.certificate = GetRawPemString("cert2.crt");
        cert.private_key = GetRawPemString("cert2.key");
        cert.sni_filters.push_back("*.cert2.com");
        options.mutable_ssl_options()->certs.push_back(cert);
    }
    EchoServiceImpl echo_svc;
    ASSERT_EQ(0, server.AddService(
            &echo_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(port, &options));

    CheckCert("cert1.com", "cert1");
    CheckCert("www.cert2.com", "cert2");
    CheckCert("noexist", "cert1");    // default cert

    server.Stop(0);
    server.Join();
}

TEST_F(SSLTest, ssl_reload) {
    const int port = 8613;
    melon::rpc::Server server;
    melon::rpc::ServerOptions options;
    {
        melon::rpc::CertInfo cert;
        cert.certificate = "cert1.crt";
        cert.private_key = "cert1.key";
        cert.sni_filters.push_back("cert1.com");
        options.mutable_ssl_options()->default_cert = cert;
    }
    EchoServiceImpl echo_svc;
    ASSERT_EQ(0, server.AddService(
            &echo_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(port, &options));

    CheckCert("cert2.com", "cert1");    // default cert
    {
        melon::rpc::CertInfo cert;
        cert.certificate = GetRawPemString("cert2.crt");
        cert.private_key = GetRawPemString("cert2.key");
        cert.sni_filters.push_back("cert2.com");
        ASSERT_EQ(0, server.AddCertificate(cert));
    }
    CheckCert("cert2.com", "cert2");

    {
        melon::rpc::CertInfo cert;
        cert.certificate = GetRawPemString("cert2.crt");
        cert.private_key = GetRawPemString("cert2.key");
        ASSERT_EQ(0, server.RemoveCertificate(cert));
    }
    CheckCert("cert2.com", "cert1");    // default cert after remove cert2

    {
        melon::rpc::CertInfo cert;
        cert.certificate = GetRawPemString("cert2.crt");
        cert.private_key = GetRawPemString("cert2.key");
        cert.sni_filters.push_back("cert2.com");
        std::vector<melon::rpc::CertInfo> certs;
        certs.push_back(cert);
        ASSERT_EQ(0, server.ResetCertificates(certs));
    }
    CheckCert("cert2.com", "cert2");

    server.Stop(0);
    server.Join();
}

#endif  // SSL_CTRL_SET_TLSEXT_HOSTNAME

const int BUFSIZE[] = {64, 128, 256, 1024, 4096};
const int REP = 100000;

void *ssl_perf_client(void *arg) {
    SSL *ssl = (SSL *) arg;
    EXPECT_EQ(1, SSL_do_handshake(ssl));

    char buf[4096];
    melon::stop_watcher tm;
    for (size_t i = 0; i < MELON_ARRAY_SIZE(BUFSIZE); ++i) {
        int size = BUFSIZE[i];
        tm.start();
        for (int j = 0; j < REP; ++j) {
            SSL_write(ssl, buf, size);
        }
        tm.stop();
        MELON_LOG(INFO) << "SSL_write(" << size << ") tp="
                  << size * REP / tm.u_elapsed() << "M/s"
                  << ", latency=" << tm.u_elapsed() / REP << "us";
    }
    return nullptr;
}

void *ssl_perf_server(void *arg) {
    SSL *ssl = (SSL *) arg;
    EXPECT_EQ(1, SSL_do_handshake(ssl));
    char buf[4096];
    for (size_t i = 0; i < MELON_ARRAY_SIZE(BUFSIZE); ++i) {
        int size = BUFSIZE[i];
        for (int j = 0; j < REP; ++j) {
            SSL_read(ssl, buf, size);
        }
    }
    return nullptr;
}

TEST_F(SSLTest, ssl_perf) {
    const melon::base::end_point ep(melon::base::IP_ANY, 5961);
    melon::base::fd_guard listenfd(melon::base::tcp_listen(ep));
    ASSERT_GT(listenfd, 0);
    int clifd = tcp_connect(ep, nullptr);
    ASSERT_GT(clifd, 0);
    int servfd = accept(listenfd, nullptr, nullptr);
    ASSERT_GT(servfd, 0);

    melon::rpc::ChannelSSLOptions opt;
    SSL_CTX *cli_ctx = melon::rpc::CreateClientSSLContext(opt);
    SSL_CTX *serv_ctx =
            melon::rpc::CreateServerSSLContext("cert1.crt", "cert1.key",
                                               melon::rpc::SSLOptions(), nullptr);
    SSL *cli_ssl = melon::rpc::CreateSSLSession(cli_ctx, 0, clifd, false);
#if defined(SSL_CTRL_SET_TLSEXT_HOSTNAME)
    SSL_set_tlsext_host_name(cli_ssl, "localhost");
#endif
    SSL *serv_ssl = melon::rpc::CreateSSLSession(serv_ctx, 0, servfd, true);
    pthread_t cpid;
    pthread_t spid;
    ASSERT_EQ(0, pthread_create(&cpid, nullptr, ssl_perf_client, cli_ssl));
    ASSERT_EQ(0, pthread_create(&spid, nullptr, ssl_perf_server, serv_ssl));
    ASSERT_EQ(0, pthread_join(cpid, nullptr));
    ASSERT_EQ(0, pthread_join(spid, nullptr));
    close(clifd);
    close(servfd);
}
