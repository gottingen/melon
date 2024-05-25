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


#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/server.h>
#include <melon/rpc/interceptor.h>
#include "echo.pb.h"

namespace melon {
namespace policy {
DECLARE_bool(use_http_error_code);
}
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}

const int EREJECT = 4000;
int g_index = 0;
const int port = 8613;
const std::string EXP_REQUEST = "hello";
const std::string EXP_RESPONSE = "world";

class EchoServiceImpl : public ::test::EchoService {
public:
    EchoServiceImpl() = default;
    ~EchoServiceImpl() override = default;
    void Echo(google::protobuf::RpcController* cntl_base,
              const ::test::EchoRequest* request,
              ::test::EchoResponse* response,
              google::protobuf::Closure* done) override {
        melon::ClosureGuard done_guard(done);
        ASSERT_EQ(EXP_REQUEST, request->message());
        response->set_message(EXP_RESPONSE);
    }
};


class MyInterceptor : public melon::Interceptor {
public:
    MyInterceptor() = default;

    ~MyInterceptor() override = default;

    bool Accept(const melon::Controller* controller,
                int& error_code,
                std::string& error_txt) const override {
        if (g_index % 2 == 0) {
            error_code = EREJECT;
            error_txt = "reject g_index=0";
            return false;
        }

        return true;
    }
};

class InterceptorTest : public ::testing::Test {
public:
    InterceptorTest() {
        EXPECT_EQ(0, _server.AddService(&_echo_svc,
                                        melon::SERVER_DOESNT_OWN_SERVICE));
        melon::ServerOptions options;
        options.interceptor = new MyInterceptor;
        options.server_owns_interceptor = true;
        EXPECT_EQ(0, _server.Start(port, &options));
    }

    ~InterceptorTest() override = default;

    static void CallMethod(test::EchoService_Stub& stub,
                           ::test::EchoRequest& req,
                           ::test::EchoResponse& res) {
        for (g_index = 0; g_index < 1000; ++g_index) {
            melon::Controller cntl;
            stub.Echo(&cntl, &req, &res, NULL);
            if (g_index % 2 == 0) {
                ASSERT_TRUE(cntl.Failed());
                ASSERT_EQ(EREJECT, cntl.ErrorCode());
            } else {
                ASSERT_FALSE(cntl.Failed());
                EXPECT_EQ(EXP_RESPONSE, res.message()) << cntl.ErrorText();
            }
        }
    }

private:
    melon::Server _server;
    EchoServiceImpl _echo_svc;
};

TEST_F(InterceptorTest, sanity) {
    ::test::EchoRequest req;
    ::test::EchoResponse res;
    req.set_message(EXP_REQUEST);

    // PROTOCOL_MELON_STD
    {
        melon::Channel channel;
        melon::ChannelOptions options;
        ASSERT_EQ(0, channel.Init("localhost", port, &options));
        test::EchoService_Stub stub(&channel);
        CallMethod(stub, req, res);
    }

    // PROTOCOL_HTTP
    {
        melon::Channel channel;
        melon::ChannelOptions options;
        options.protocol = melon::PROTOCOL_HTTP;
        ASSERT_EQ(0, channel.Init("localhost", port, &options));
        test::EchoService_Stub stub(&channel);
        // Set the x-bd-error-code header of http response to melon error code.
        melon::policy::FLAGS_use_http_error_code = true;
        CallMethod(stub, req, res);
    }

    // PROTOCOL_HULU_PBRPC
    {
        melon::Channel channel;
        melon::ChannelOptions options;
        options.protocol = melon::PROTOCOL_HULU_PBRPC;
        ASSERT_EQ(0, channel.Init("localhost", port, &options));
        test::EchoService_Stub stub(&channel);
        CallMethod(stub, req, res);
    }
}