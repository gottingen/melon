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




// Date: 2015/10/22 16:28:44

#include <gtest/gtest.h>

#include <melon/rpc/server.h>
#include <melon/rpc/controller.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/stream_impl.h>
#include "echo.pb.h"

class AfterAcceptStream {
public:
    virtual void action(melon::StreamId) = 0;
};

class MyServiceWithStream : public test::EchoService {
public:
    MyServiceWithStream(const melon::StreamOptions& options)
        : _options(options)
        , _after_accept_stream(NULL)
    {}
    MyServiceWithStream(const melon::StreamOptions& options,
                        AfterAcceptStream* after_accept_stream) 
        : _options(options)
        , _after_accept_stream(after_accept_stream)
    {}
    MyServiceWithStream()
        : _options()
        , _after_accept_stream(NULL)
    {}

    void Echo(::google::protobuf::RpcController* controller,
                       const ::test::EchoRequest* request,
                       ::test::EchoResponse* response,
                       ::google::protobuf::Closure* done) {
        melon::ClosureGuard done_gurad(done);
        response->set_message(request->message());
        melon::Controller* cntl = (melon::Controller*)controller;
        melon::StreamId response_stream;
        ASSERT_EQ(0, StreamAccept(&response_stream, *cntl, &_options));
        LOG(INFO) << "Created response_stream=" << response_stream;
        if (_after_accept_stream) {
            _after_accept_stream->action(response_stream);
        }
    }
private:
    melon::StreamOptions _options;
    AfterAcceptStream* _after_accept_stream;
};

class StreamingRpcTest : public testing::Test {
protected:
    test::EchoRequest request;
    test::EchoResponse response;
    void SetUp() { request.set_message("hello world"); }
    void TearDown() {}
};

TEST_F(StreamingRpcTest, sanity) {
    melon::Server server;
    MyServiceWithStream service;
    ASSERT_EQ(0, server.AddService(&service, melon::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    melon::Controller cntl;
    melon::StreamId request_stream;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, NULL));
    melon::ScopedStream stream_guard(request_stream);
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" << request_stream;
    usleep(10);
    melon::StreamClose(request_stream);
    server.Stop(0);
    server.Join();
}

struct HandlerControl {
    HandlerControl() 
        : block(false)
    {}
    bool block;
};

class OrderedInputHandler : public melon::StreamInputHandler {
public:
    explicit OrderedInputHandler(HandlerControl *cntl = NULL)
        : _expected_next_value(0)
        , _failed(false)
        , _stopped(false)
        , _idle_times(0)
        , _cntl(cntl)
    {
    }
    int on_received_messages(melon::StreamId /*id*/,
                             mutil::IOBuf *const messages[],
                             size_t size) {
        if (_cntl && _cntl->block) {
            while (_cntl->block) {
                usleep(100);
            }
        }
        for (size_t i = 0; i < size; ++i) {
            CHECK(messages[i]->length() == sizeof(int));
            int network = 0;
            messages[i]->cutn(&network, sizeof(int));
            EXPECT_EQ((int)ntohl(network), _expected_next_value++);
        }
        return 0;
    }

    void on_idle_timeout(melon::StreamId /*id*/) {
        ++_idle_times;
    }

    void on_closed(melon::StreamId /*id*/) {
        ASSERT_FALSE(_stopped);
        _stopped = true;
    }

    bool failed() const { return _failed; }
    bool stopped() const { return _stopped; }
    int idle_times() const { return _idle_times; }
private:
    int _expected_next_value;
    bool _failed;
    bool _stopped;
    int _idle_times;
    HandlerControl* _cntl;
};

TEST_F(StreamingRpcTest, received_in_order) {
    OrderedInputHandler handler;
    melon::StreamOptions opt;
    opt.handler = &handler;
    opt.messages_in_batch = 100;
    melon::Server server;
    MyServiceWithStream service(opt);
    ASSERT_EQ(0, server.AddService(&service, melon::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    melon::Controller cntl;
    melon::StreamId request_stream;
    melon::StreamOptions request_stream_options;
    request_stream_options.max_buf_size = 0;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, &request_stream_options));
    melon::ScopedStream stream_guard(request_stream);
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" << request_stream;
    const int N = 10000;
    for (int i = 0; i < N; ++i) {
        int network = htonl(i);
        mutil::IOBuf out;
        out.append(&network, sizeof(network));
        ASSERT_EQ(0, melon::StreamWrite(request_stream, out)) << "i=" << i;
    }
    ASSERT_EQ(0, melon::StreamClose(request_stream));
    server.Stop(0);
    server.Join();
    while (!handler.stopped()) {
        usleep(100);
    }
    ASSERT_FALSE(handler.failed());
    ASSERT_EQ(0, handler.idle_times());
    ASSERT_EQ(N, handler._expected_next_value);
}

void on_writable(melon::StreamId, void* arg, int error_code) {
    std::pair<bool, int>* p = (std::pair<bool, int>*)arg;
    p->first = true;
    p->second = error_code;
    LOG(INFO) << "error_code=" << error_code;
}

TEST_F(StreamingRpcTest, block) {
    HandlerControl hc;
    OrderedInputHandler handler(&hc);
    hc.block = true;
    melon::StreamOptions opt;
    opt.handler = &handler;
    const int N = 10000;
    opt.max_buf_size = sizeof(uint32_t) *  N;
    melon::Server server;
    MyServiceWithStream service(opt);
    ASSERT_EQ(0, server.AddService(&service, melon::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    melon::Controller cntl;
    melon::StreamId request_stream;
    melon::ScopedStream stream_guard(request_stream);
    melon::StreamOptions request_stream_options;
    request_stream_options.max_buf_size = sizeof(uint32_t) * N;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, &request_stream_options));
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" 
                                << request_stream;
    for (int i = 0; i < N; ++i) {
        int network = htonl(i);
        mutil::IOBuf out;
        out.append(&network, sizeof(network));
        ASSERT_EQ(0, melon::StreamWrite(request_stream, out)) << "i=" << i;
    }
    // sync wait
    int dummy = 102030123;
    mutil::IOBuf out;
    out.append(&dummy, sizeof(dummy));
    ASSERT_EQ(EAGAIN, melon::StreamWrite(request_stream, out));
    hc.block = false;
    ASSERT_EQ(0, melon::StreamWait(request_stream, NULL));
    // wait flushing all the pending messages
    while (handler._expected_next_value != N) {
        usleep(100);
    }
    // block hanlder again to test async wait
    hc.block = true;
    // async wait
    for (int i = N; i < N + N; ++i) {
        int network = htonl(i);
        mutil::IOBuf out;
        out.append(&network, sizeof(network));
        ASSERT_EQ(0, melon::StreamWrite(request_stream, out)) << "i=" << i;
    }
    out.clear();
    out.append(&dummy, sizeof(dummy));
    ASSERT_EQ(EAGAIN, melon::StreamWrite(request_stream, out));
    hc.block = false;
    std::pair<bool, int> p = std::make_pair(false, 0);
    usleep(10);
    melon::StreamWait(request_stream, NULL, on_writable, &p);
    while (!p.first) {
        usleep(100);
    }
    ASSERT_EQ(0, p.second);

    // wait flushing all the pending messages
    while (handler._expected_next_value != N + N) {
        usleep(100);
    }
    usleep(1000);

    LOG(INFO) << "Starting block";
    hc.block = true;
    for (int i = N + N; i < N + N + N; ++i) {
        int network = htonl(i);
        mutil::IOBuf out;
        out.append(&network, sizeof(network));
        ASSERT_EQ(0, melon::StreamWrite(request_stream, out)) << "i=" << i - N - N;
    }
    out.clear();
    out.append(&dummy, sizeof(dummy));
    ASSERT_EQ(EAGAIN, melon::StreamWrite(request_stream, out));
    timespec duetime = mutil::microseconds_from_now(1);
    p.first = false;
    LOG(INFO) << "Start wait";
    melon::StreamWait(request_stream, &duetime, on_writable, &p);
    while (!p.first) {
        usleep(100);
    }
    ASSERT_TRUE(p.first);
    EXPECT_EQ(ETIMEDOUT, p.second);
    hc.block = false;
    ASSERT_EQ(0, melon::StreamClose(request_stream));
    while (!handler.stopped()) {
        usleep(100);
    }

    ASSERT_FALSE(handler.failed());
    ASSERT_EQ(0, handler.idle_times());
    ASSERT_EQ(N + N + N, handler._expected_next_value);
}

TEST_F(StreamingRpcTest, auto_close_if_host_socket_closed) {
    HandlerControl hc;
    OrderedInputHandler handler(&hc);
    hc.block = true;
    melon::StreamOptions opt;
    opt.handler = &handler;
    const int N = 10000;
    opt.max_buf_size = sizeof(uint32_t) *  N;
    melon::Server server;
    MyServiceWithStream service(opt);
    ASSERT_EQ(0, server.AddService(&service, melon::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    melon::Controller cntl;
    melon::StreamId request_stream;
    melon::StreamOptions request_stream_options;
    request_stream_options.max_buf_size = sizeof(uint32_t) * N;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, &request_stream_options));
    melon::ScopedStream stream_guard(request_stream);
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" << request_stream;

    {
        melon::SocketUniquePtr ptr;
        ASSERT_EQ(0, melon::Socket::Address(request_stream, &ptr));
        melon::Stream* s = (melon::Stream*)ptr->conn();
        ASSERT_TRUE(s->_host_socket != NULL);
        s->_host_socket->SetFailed();
    }

    usleep(100);
    mutil::IOBuf out;
    out.append("test");
    ASSERT_EQ(EINVAL, melon::StreamWrite(request_stream, out));
    while (!handler.stopped()) {
        usleep(100);
    }
    ASSERT_FALSE(handler.failed());
    ASSERT_EQ(0, handler.idle_times());
    ASSERT_EQ(0, handler._expected_next_value);
}

TEST_F(StreamingRpcTest, idle_timeout) {
    HandlerControl hc;
    OrderedInputHandler handler(&hc);
    hc.block = true;
    melon::StreamOptions opt;
    opt.handler = &handler;
    opt.idle_timeout_ms = 2;
    const int N = 10000;
    opt.max_buf_size = sizeof(uint32_t) *  N;
    melon::Server server;
    MyServiceWithStream service(opt);
    ASSERT_EQ(0, server.AddService(&service, melon::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    melon::Controller cntl;
    melon::StreamId request_stream;
    melon::StreamOptions request_stream_options;
    request_stream_options.max_buf_size = sizeof(uint32_t) * N;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, &request_stream_options));
    melon::ScopedStream stream_guard(request_stream);
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" << request_stream;
    usleep(10 * 1000 + 800);
    ASSERT_EQ(0, melon::StreamClose(request_stream));
    while (!handler.stopped()) {
        usleep(100);
    }
    ASSERT_FALSE(handler.failed());
//    ASSERT_TRUE(handler.idle_times() >= 4 && handler.idle_times() <= 6)
//               << handler.idle_times();
    ASSERT_EQ(0, handler._expected_next_value);
}

class PingPongHandler : public melon::StreamInputHandler {
public:
    explicit PingPongHandler()
        : _expected_next_value(0)
        , _failed(false)
        , _stopped(false)
        , _idle_times(0)
    {
    }
    int on_received_messages(melon::StreamId id,
                             mutil::IOBuf *const messages[],
                             size_t size) {
        if (size != 1) {
            _failed = true;
            return 0;
        }
        for (size_t i = 0; i < size; ++i) {
            CHECK(messages[i]->length() == sizeof(int));
            int network = 0;
            messages[i]->cutn(&network, sizeof(int));
            if ((int)ntohl(network) != _expected_next_value) {
                _failed = true;
            }
            int send_back = ntohl(network) + 1;
            _expected_next_value = send_back + 1;
            mutil::IOBuf out;
            network = htonl(send_back);
            out.append(&network, sizeof(network));
            // don't care the return value
            melon::StreamWrite(id, out);
        }
        return 0;
    }

    void on_idle_timeout(melon::StreamId /*id*/) {
        ++_idle_times;
    }

    void on_closed(melon::StreamId /*id*/) {
        ASSERT_FALSE(_stopped);
        _stopped = true;
    }

    bool failed() const { return _failed; }
    bool stopped() const { return _stopped; }
    int idle_times() const { return _idle_times; }
private:
    int _expected_next_value;
    bool _failed;
    bool _stopped;
    int _idle_times;
};

TEST_F(StreamingRpcTest, ping_pong) {
    PingPongHandler resh;
    melon::StreamOptions opt;
    opt.handler = &resh;
    const int N = 10000;
    opt.max_buf_size = sizeof(uint32_t) *  N;
    melon::Server server;
    MyServiceWithStream service(opt);
    ASSERT_EQ(0, server.AddService(&service, melon::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    melon::Controller cntl;
    melon::StreamId request_stream;
    melon::StreamOptions request_stream_options;
    PingPongHandler reqh;
    reqh._expected_next_value = 1;
    request_stream_options.handler = &reqh;
    request_stream_options.max_buf_size = sizeof(uint32_t) * N;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, &request_stream_options));
    melon::ScopedStream stream_guard(request_stream);
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" << request_stream;
    int send = 0;
    mutil::IOBuf out;
    out.append(&send, sizeof(send));
    ASSERT_EQ(0, melon::StreamWrite(request_stream, out));
    usleep(10 * 1000);
    ASSERT_EQ(0, melon::StreamClose(request_stream));
    while (!resh.stopped() || !reqh.stopped()) {
        usleep(100);
    }
    ASSERT_FALSE(resh.failed());
    ASSERT_FALSE(reqh.failed());
    ASSERT_EQ(0, resh.idle_times());
    ASSERT_EQ(0, reqh.idle_times());
}

class SendNAfterAcceptStream : public AfterAcceptStream {
public:
    explicit SendNAfterAcceptStream(int n)
        : _n(n) {}
    void action(melon::StreamId s) {
        for (int i = 0; i < _n; ++i) {
            int network = htonl(i);
            mutil::IOBuf out;
            out.append(&network, sizeof(network));
            ASSERT_EQ(0, melon::StreamWrite(s, out)) << "i=" << i;
        }
    }
private:
    int _n;
};

TEST_F(StreamingRpcTest, server_send_data_before_run_done) {
    const int N = 10000;
    SendNAfterAcceptStream after_accept(N);
    melon::StreamOptions opt;
    opt.max_buf_size = -1;
    melon::Server server;
    MyServiceWithStream service(opt, &after_accept);
    ASSERT_EQ(0, server.AddService(&service, melon::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    OrderedInputHandler handler;
    melon::StreamOptions request_stream_options;
    melon::StreamId request_stream;
    melon::Controller cntl;
    request_stream_options.handler = &handler;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, &request_stream_options));
    melon::ScopedStream stream_guard(request_stream);
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" << request_stream;
    // wait flushing all the pending messages
    while (handler._expected_next_value != N) {
        usleep(100);
    }
    ASSERT_EQ(0, melon::StreamClose(request_stream));
    while (!handler.stopped()) {
        usleep(100);
    }
    ASSERT_FALSE(handler.failed());
    ASSERT_EQ(0, handler.idle_times());
}
