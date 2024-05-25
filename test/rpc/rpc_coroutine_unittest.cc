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
#include <melon/rpc/server.h>
#include <melon/rpc/channel.h>
#include "melon/rpc/coroutine.h"
#include "echo.pb.h"

int main(int argc, char* argv[]) {
#ifdef MELON_ENABLE_COROUTINE
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
#else
    printf("melon coroutine is not enabled, please add -std=c++20 to compile options\n");
    return 0;
#endif
}

#ifdef MELON_ENABLE_COROUTINE

using melon::experimental::Awaitable;
using melon::experimental::AwaitableDone;
using melon::experimental::Coroutine;

class Trace {
public:
    Trace(const std::string& name) {
        _name = name;
        MLOG(INFO) << "enter " << name;
    }
    ~Trace() {
        MLOG(INFO) << "exit " << _name;
    }
private:
    std::string _name;
};

class EchoServiceImpl : public test::EchoService {
public:
    EchoServiceImpl() {}
    virtual ~EchoServiceImpl() {}
    virtual void Echo(google::protobuf::RpcController* cntl_base,
                      const test::EchoRequest* request,
                      test::EchoResponse* response,
                      google::protobuf::Closure* done) {
        // melon::Controller* cntl = (melon::Controller*)cntl_base;
        // melon::ClosureGuard done_guard(done);
        // response->set_message(request->message());

        // Create a detached coroutine, so the current fiber will return at once.
        Coroutine(EchoAsync(request, response, done), true);
    }

    Awaitable<void> EchoAsync(const test::EchoRequest* request,
                                   test::EchoResponse* response,
                                   google::protobuf::Closure* done) {
        Trace t("EchoAsync");
        // This is important to test RAII object's destruction after coroutine finished
        melon::ClosureGuard done_guard(done);
        if (request->has_sleep_us()) {
            MLOG(INFO) << "sleep " << request->sleep_us() << " us at server side";
            co_await Coroutine::usleep(request->sleep_us());
        }
        response->set_message(request->message());
    }
};

class CoroutineTest : public ::testing::Test{
protected:
    CoroutineTest() {};
    virtual ~CoroutineTest(){};
    virtual void SetUp() {};
    virtual void TearDown() {};
};


static int delay_us = 0;

Awaitable<std::string> inplace_func(const std::string& input) {
    Trace t("inplace_func");
    co_return input;
}

Awaitable<double> inplace_func2() {
    Trace t("inplace_func2");
    co_await inplace_func("123");
    co_return 0.5;
}

Awaitable<int> sleep_func() {
    Trace t("sleep_func");
    int64_t s = mutil::monotonic_time_us();
    auto aw = Coroutine::usleep(1000);
    usleep(delay_us);
    co_await aw;
    int cost = mutil::monotonic_time_us() - s;
    EXPECT_GE(cost, 1000);
    MLOG(INFO) << "after usleep:" << cost;
    co_return 123;
}

Awaitable<float> exception_func() {
    Trace t("exception_func");
    throw std::string("error");
}

Awaitable<void> func(melon::Channel& channel, int* out) {
    Trace t("func");
    test::EchoService_Stub stub(&channel);
    test::EchoRequest request;
    request.set_message("hello world");
    test::EchoResponse response;
    melon::Controller cntl;

    MLOG(INFO) << "before start coroutine";
    Coroutine coro(sleep_func());
    usleep(delay_us);
    MLOG(INFO) << "before wait coroutine";
    int ret = co_await coro.awaitable<int>();
    EXPECT_EQ(ret, 123);
    MLOG(INFO) << "after wait coroutine, ret:" << ret;

    auto str = co_await inplace_func("hello");
    EXPECT_EQ("hello", str);

    float num = 0.0;
    try {
        num = co_await exception_func();
    } catch(std::string str) {
        EXPECT_EQ("error", str);
        num = 1.0;
    }
    EXPECT_EQ(1.0, num);

    AwaitableDone done;
    MLOG(INFO) << "start echo";
    stub.Echo(&cntl, &request, &response, &done);
    MLOG(INFO) << "after echo";
    usleep(delay_us);
    co_await done.awaitable();
    MLOG(INFO) << "after wait";
    EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
    EXPECT_EQ("hello world", response.message());

    cntl.Reset();
    request.set_sleep_us(2000);
    AwaitableDone done2;
    MLOG(INFO) << "start echo2";
    int64_t s = mutil::monotonic_time_us();
    stub.Echo(&cntl, &request, &response, &done2);
    MLOG(INFO) << "after echo2";
    co_await done2.awaitable();
    int cost = mutil::monotonic_time_us() - s;
    MLOG(INFO) << "after wait2";
    EXPECT_GE(cost, 2000);
    EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
    EXPECT_EQ("hello world", response.message());

    *out = 456;
}

TEST_F(CoroutineTest, coroutine) {
    mutil::EndPoint ep;
    ASSERT_EQ(0, str2endpoint("127.0.0.1:8613", &ep));

    melon::Server server;
    EchoServiceImpl service;
    server.AddService(&service, melon::SERVER_DOESNT_OWN_SERVICE);
    ASSERT_EQ(0, server.Start(ep, NULL));

    melon::Channel channel;
    melon::ChannelOptions options;
    ASSERT_EQ(0, channel.Init(ep, &options));

    int out = 0;
    Coroutine coro(func(channel, &out));
    coro.join();
    ASSERT_EQ(456, out);

    out = 0;
    delay_us = 10000;
    Coroutine coro2(func(channel, &out));
    coro2.join();
    ASSERT_EQ(456, out);
    delay_us = 0;

    Coroutine coro3(inplace_func2());
    double d = coro3.join<double>();
    ASSERT_EQ(0.5, d);

    Coroutine coro4(inplace_func("abc"));
    coro4.join();

    Coroutine coro5(sleep_func());
    coro5.join();

    Coroutine coro6(inplace_func2(), true);
    Coroutine coro7(inplace_func("abc"), true);
    Coroutine coro8(sleep_func(), true);
    usleep(10000); // wait sleep_func() to complete

    MLOG(INFO) << "test case finished";
}

#endif // MELON_ENABLE_COROUTINE