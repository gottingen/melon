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


#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "gflags/gflags.h"

#include "echo.pb.h"
#include <melon/rpc/channel.h>
#include <melon/rpc/server.h>
#include <melon/utility/fd_guard.h>
#include <melon/utility/endpoint.h>

DEFINE_string(listen_addr, "0.0.0.0:8011", "Server listen address.");

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);    
    return RUN_ALL_TESTS();
}

namespace {

class EchoServerImpl : public test::EchoService {
public:
    virtual void Echo(google::protobuf::RpcController* controller,
                        const ::test::EchoRequest* request,
                        test::EchoResponse* response,
                        google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);
        response->set_message(request->message());

        melon::Controller* cntl = static_cast<melon::Controller*>(controller);
        LOG(INFO) << "protocol:" << cntl->request_protocol();
  }
};

class ALPNTest : public testing::Test {
public:
    ALPNTest() = default;
    virtual ~ALPNTest() = default;

    virtual void SetUp() override { 
        // Start melon server with SSL
        melon::ServerOptions server_options;
        auto&& ssl_options = server_options.mutable_ssl_options();
        ssl_options->default_cert.certificate = "cert1.crt";
        ssl_options->default_cert.private_key = "cert1.key";
        ssl_options->alpns = "http, h2, melon_std";

        EXPECT_EQ(0, _server.AddService(&_echo_server_impl,
                                        melon::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, _server.Start(FLAGS_listen_addr.data(), &server_options));
    }
    
    virtual void TearDown() override {
        _server.Stop(0);
        _server.Join();
    }

    std::string HandshakeWithServer(std::vector<std::string> alpns) {
        // Init client ssl ctx and set alpn.
        melon::ChannelSSLOptions options;
        SSL_CTX* ssl_ctx = melon::CreateClientSSLContext(options);
        EXPECT_NE(nullptr, ssl_ctx);

        std::string raw_alpn;
        for (auto&& alpn : alpns) {
            raw_alpn.append(melon::ALPNProtocolToString(melon::AdaptiveProtocolType(alpn)));
        }
        SSL_CTX_set_alpn_protos(ssl_ctx, 
                reinterpret_cast<const unsigned char*>(raw_alpn.data()), raw_alpn.size());
    
        // TCP connect.
        mutil::EndPoint endpoint;
        mutil::str2endpoint(FLAGS_listen_addr.data(), &endpoint);

        int cli_fd = mutil::tcp_connect(endpoint, nullptr);
        mutil::fd_guard guard(cli_fd);
        EXPECT_NE(0, cli_fd);

        // SSL handshake.
        SSL* ssl = melon::CreateSSLSession(ssl_ctx, 0, cli_fd, false);
        EXPECT_NE(nullptr, ssl);
        EXPECT_EQ(1, SSL_do_handshake(ssl)); 

        // Get handshake result.
        const unsigned char* select_alpn = nullptr;
        unsigned int len = 0;
        SSL_get0_alpn_selected(ssl, &select_alpn, &len);
        return std::string(reinterpret_cast<const char*>(select_alpn), len);
    }

private:
    melon::Server _server;
    EchoServerImpl _echo_server_impl;
};

TEST_F(ALPNTest, Server) {
    // Server alpn support h2 http melon_std, test the following case:
    // 1. Client provides 1 protocol which is in the list supported by the server.
    // 2. Server select protocol according to priority.
    // 3. Server does not support the protocol provided by the client.

    EXPECT_EQ("melon_std", ALPNTest::HandshakeWithServer({"melon_std"}));
    EXPECT_EQ("h2", ALPNTest::HandshakeWithServer({"melon_std", "h2"}));
    EXPECT_EQ("", ALPNTest::HandshakeWithServer({"nshead"}));
}

} // namespace
