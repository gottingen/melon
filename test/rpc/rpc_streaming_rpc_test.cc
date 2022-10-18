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



// Date: 2015/10/22 16:28:44

#include "testing/gtest_wrap.h"

#include "melon/rpc/server.h"
#include "melon/rpc/controller.h"
#include "melon/rpc/channel.h"
#include "melon/rpc/stream_impl.h"
#include "echo.pb.h"

class AfterAcceptStream {
public:
    virtual void action(melon::rpc::StreamId) = 0;
};

class MyServiceWithStream : public test::EchoService {
public:
    MyServiceWithStream(const melon::rpc::StreamOptions& options)
        : _options(options)
        , _after_accept_stream(NULL)
    {}
    MyServiceWithStream(const melon::rpc::StreamOptions& options,
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
        melon::rpc::ClosureGuard done_gurad(done);
        response->set_message(request->message());
        melon::rpc::Controller* cntl = (melon::rpc::Controller*)controller;
        melon::rpc::StreamId response_stream;
        ASSERT_EQ(0, StreamAccept(&response_stream, *cntl, &_options));
        MELON_LOG(INFO) << "Created response_stream=" << response_stream;
        if (_after_accept_stream) {
            _after_accept_stream->action(response_stream);
        }
    }
private:
    melon::rpc::StreamOptions _options;
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
    melon::rpc::Server server;
    MyServiceWithStream service;
    ASSERT_EQ(0, server.AddService(&service, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::rpc::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    melon::rpc::Controller cntl;
    melon::rpc::StreamId request_stream;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, NULL));
    melon::rpc::ScopedStream stream_guard(request_stream);
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" << request_stream;
    usleep(10);
    melon::rpc::StreamClose(request_stream);
    server.Stop(0);
    server.Join();
}

struct HandlerControl {
    HandlerControl() 
        : block(false)
    {}
    bool block;
};

class OrderedInputHandler : public melon::rpc::StreamInputHandler {
public:
    explicit OrderedInputHandler(HandlerControl *cntl = NULL)
        : _expected_next_value(0)
        , _failed(false)
        , _stopped(false)
        , _idle_times(0)
        , _cntl(cntl)
    {
    }
    int on_received_messages(melon::rpc::StreamId /*id*/,
                             melon::cord_buf *const messages[],
                             size_t size) {
        if (_cntl && _cntl->block) {
            while (_cntl->block) {
                usleep(100);
            }
        }
        for (size_t i = 0; i < size; ++i) {
            MELON_CHECK(messages[i]->length() == sizeof(int));
            int network = 0;
            messages[i]->cutn(&network, sizeof(int));
            EXPECT_EQ((int)ntohl(network), _expected_next_value++);
        }
        return 0;
    }

    void on_idle_timeout(melon::rpc::StreamId /*id*/) {
        ++_idle_times;
    }

    void on_closed(melon::rpc::StreamId /*id*/) {
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
    melon::rpc::StreamOptions opt;
    opt.handler = &handler;
    opt.messages_in_batch = 100;
    melon::rpc::Server server;
    MyServiceWithStream service(opt);
    ASSERT_EQ(0, server.AddService(&service, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::rpc::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    melon::rpc::Controller cntl;
    melon::rpc::StreamId request_stream;
    melon::rpc::StreamOptions request_stream_options;
    request_stream_options.max_buf_size = 0;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, &request_stream_options));
    melon::rpc::ScopedStream stream_guard(request_stream);
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" << request_stream;
    const int N = 10000;
    for (int i = 0; i < N; ++i) {
        int network = htonl(i);
        melon::cord_buf out;
        out.append(&network, sizeof(network));
        ASSERT_EQ(0, melon::rpc::StreamWrite(request_stream, out)) << "i=" << i;
    }
    ASSERT_EQ(0, melon::rpc::StreamClose(request_stream));
    server.Stop(0);
    server.Join();
    while (!handler.stopped()) {
        usleep(100);
    }
    ASSERT_FALSE(handler.failed());
    ASSERT_EQ(0, handler.idle_times());
    ASSERT_EQ(N, handler._expected_next_value);
}

void on_writable(melon::rpc::StreamId, void* arg, int error_code) {
    std::pair<bool, int>* p = (std::pair<bool, int>*)arg;
    p->first = true;
    p->second = error_code;
    MELON_LOG(INFO) << "error_code=" << error_code;
}

TEST_F(StreamingRpcTest, block) {
    HandlerControl hc;
    OrderedInputHandler handler(&hc);
    hc.block = true;
    melon::rpc::StreamOptions opt;
    opt.handler = &handler;
    const int N = 10000;
    opt.max_buf_size = sizeof(uint32_t) *  N;
    melon::rpc::Server server;
    MyServiceWithStream service(opt);
    ASSERT_EQ(0, server.AddService(&service, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::rpc::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    melon::rpc::Controller cntl;
    melon::rpc::StreamId request_stream;
    melon::rpc::ScopedStream stream_guard(request_stream);
    melon::rpc::StreamOptions request_stream_options;
    request_stream_options.max_buf_size = sizeof(uint32_t) * N;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, &request_stream_options));
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" 
                                << request_stream;
    for (int i = 0; i < N; ++i) {
        int network = htonl(i);
        melon::cord_buf out;
        out.append(&network, sizeof(network));
        ASSERT_EQ(0, melon::rpc::StreamWrite(request_stream, out)) << "i=" << i;
    }
    // sync wait
    int dummy = 102030123;
    melon::cord_buf out;
    out.append(&dummy, sizeof(dummy));
    ASSERT_EQ(EAGAIN, melon::rpc::StreamWrite(request_stream, out));
    hc.block = false;
    ASSERT_EQ(0, melon::rpc::StreamWait(request_stream, NULL));
    // wait flushing all the pending messages
    while (handler._expected_next_value != N) {
        usleep(100);
    }
    // block hanlder again to test async wait
    hc.block = true;
    // async wait
    for (int i = N; i < N + N; ++i) {
        int network = htonl(i);
        melon::cord_buf out;
        out.append(&network, sizeof(network));
        ASSERT_EQ(0, melon::rpc::StreamWrite(request_stream, out)) << "i=" << i;
    }
    out.clear();
    out.append(&dummy, sizeof(dummy));
    ASSERT_EQ(EAGAIN, melon::rpc::StreamWrite(request_stream, out));
    hc.block = false;
    std::pair<bool, int> p = std::make_pair(false, 0);
    usleep(10);
    melon::rpc::StreamWait(request_stream, NULL, on_writable, &p);
    while (!p.first) {
        usleep(100);
    }
    ASSERT_EQ(0, p.second);

    // wait flushing all the pending messages
    while (handler._expected_next_value != N + N) {
        usleep(100);
    }
    usleep(1000);

    MELON_LOG(INFO) << "Starting block";
    hc.block = true;
    for (int i = N + N; i < N + N + N; ++i) {
        int network = htonl(i);
        melon::cord_buf out;
        out.append(&network, sizeof(network));
        ASSERT_EQ(0, melon::rpc::StreamWrite(request_stream, out)) << "i=" << i - N - N;
    }
    out.clear();
    out.append(&dummy, sizeof(dummy));
    ASSERT_EQ(EAGAIN, melon::rpc::StreamWrite(request_stream, out));
    timespec duetime =  melon::time_point::future_unix_micros(1).to_timespec();
    p.first = false;
    MELON_LOG(INFO) << "Start wait";
    melon::rpc::StreamWait(request_stream, &duetime, on_writable, &p);
    while (!p.first) {
        usleep(100);
    }
    ASSERT_TRUE(p.first);
    EXPECT_EQ(ETIMEDOUT, p.second);
    hc.block = false;
    ASSERT_EQ(0, melon::rpc::StreamClose(request_stream));
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
    melon::rpc::StreamOptions opt;
    opt.handler = &handler;
    const int N = 10000;
    opt.max_buf_size = sizeof(uint32_t) *  N;
    melon::rpc::Server server;
    MyServiceWithStream service(opt);
    ASSERT_EQ(0, server.AddService(&service, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::rpc::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    melon::rpc::Controller cntl;
    melon::rpc::StreamId request_stream;
    melon::rpc::StreamOptions request_stream_options;
    request_stream_options.max_buf_size = sizeof(uint32_t) * N;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, &request_stream_options));
    melon::rpc::ScopedStream stream_guard(request_stream);
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" << request_stream;

    {
        melon::rpc::SocketUniquePtr ptr;
        ASSERT_EQ(0, melon::rpc::Socket::Address(request_stream, &ptr));
        melon::rpc::Stream* s = (melon::rpc::Stream*)ptr->conn();
        ASSERT_TRUE(s->_host_socket != NULL);
        s->_host_socket->SetFailed();
    }

    usleep(100);
    melon::cord_buf out;
    out.append("test");
    ASSERT_EQ(EINVAL, melon::rpc::StreamWrite(request_stream, out));
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
    melon::rpc::StreamOptions opt;
    opt.handler = &handler;
    opt.idle_timeout_ms = 2;
    const int N = 10000;
    opt.max_buf_size = sizeof(uint32_t) *  N;
    melon::rpc::Server server;
    MyServiceWithStream service(opt);
    ASSERT_EQ(0, server.AddService(&service, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::rpc::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    melon::rpc::Controller cntl;
    melon::rpc::StreamId request_stream;
    melon::rpc::StreamOptions request_stream_options;
    request_stream_options.max_buf_size = sizeof(uint32_t) * N;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, &request_stream_options));
    melon::rpc::ScopedStream stream_guard(request_stream);
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" << request_stream;
    usleep(10 * 1000 + 800);
    ASSERT_EQ(0, melon::rpc::StreamClose(request_stream));
    while (!handler.stopped()) {
        usleep(100);
    }
    ASSERT_FALSE(handler.failed());
//    ASSERT_TRUE(handler.idle_times() >= 4 && handler.idle_times() <= 6)
//               << handler.idle_times();
    ASSERT_EQ(0, handler._expected_next_value);
}

class PingPongHandler : public melon::rpc::StreamInputHandler {
public:
    explicit PingPongHandler()
        : _expected_next_value(0)
        , _failed(false)
        , _stopped(false)
        , _idle_times(0)
    {
    }
    int on_received_messages(melon::rpc::StreamId id,
                             melon::cord_buf *const messages[],
                             size_t size) {
        if (size != 1) {
            _failed = true;
            return 0;
        }
        for (size_t i = 0; i < size; ++i) {
            MELON_CHECK(messages[i]->length() == sizeof(int));
            int network = 0;
            messages[i]->cutn(&network, sizeof(int));
            if ((int)ntohl(network) != _expected_next_value) {
                _failed = true;
            }
            int send_back = ntohl(network) + 1;
            _expected_next_value = send_back + 1;
            melon::cord_buf out;
            network = htonl(send_back);
            out.append(&network, sizeof(network));
            // don't care the return value
            melon::rpc::StreamWrite(id, out);
        }
        return 0;
    }

    void on_idle_timeout(melon::rpc::StreamId /*id*/) {
        ++_idle_times;
    }

    void on_closed(melon::rpc::StreamId /*id*/) {
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
    melon::rpc::StreamOptions opt;
    opt.handler = &resh;
    const int N = 10000;
    opt.max_buf_size = sizeof(uint32_t) *  N;
    melon::rpc::Server server;
    MyServiceWithStream service(opt);
    ASSERT_EQ(0, server.AddService(&service, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::rpc::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    melon::rpc::Controller cntl;
    melon::rpc::StreamId request_stream;
    melon::rpc::StreamOptions request_stream_options;
    PingPongHandler reqh;
    reqh._expected_next_value = 1;
    request_stream_options.handler = &reqh;
    request_stream_options.max_buf_size = sizeof(uint32_t) * N;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, &request_stream_options));
    melon::rpc::ScopedStream stream_guard(request_stream);
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" << request_stream;
    int send = 0;
    melon::cord_buf out;
    out.append(&send, sizeof(send));
    ASSERT_EQ(0, melon::rpc::StreamWrite(request_stream, out));
    usleep(10 * 1000);
    ASSERT_EQ(0, melon::rpc::StreamClose(request_stream));
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
    void action(melon::rpc::StreamId s) {
        for (int i = 0; i < _n; ++i) {
            int network = htonl(i);
            melon::cord_buf out;
            out.append(&network, sizeof(network));
            ASSERT_EQ(0, melon::rpc::StreamWrite(s, out)) << "i=" << i;
        }
    }
private:
    int _n;
};

TEST_F(StreamingRpcTest, server_send_data_before_run_done) {
    const int N = 10000;
    SendNAfterAcceptStream after_accept(N);
    melon::rpc::StreamOptions opt;
    opt.max_buf_size = -1;
    melon::rpc::Server server;
    MyServiceWithStream service(opt, &after_accept);
    ASSERT_EQ(0, server.AddService(&service, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start(9007, NULL));
    melon::rpc::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1:9007", NULL));
    OrderedInputHandler handler;
    melon::rpc::StreamOptions request_stream_options;
    melon::rpc::StreamId request_stream;
    melon::rpc::Controller cntl;
    request_stream_options.handler = &handler;
    ASSERT_EQ(0, StreamCreate(&request_stream, cntl, &request_stream_options));
    melon::rpc::ScopedStream stream_guard(request_stream);
    test::EchoService_Stub stub(&channel);
    stub.Echo(&cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText() << " request_stream=" << request_stream;
    // wait flushing all the pending messages
    while (handler._expected_next_value != N) {
        usleep(100);
    }
    ASSERT_EQ(0, melon::rpc::StreamClose(request_stream));
    while (!handler.stopped()) {
        usleep(100);
    }
    ASSERT_FALSE(handler.failed());
    ASSERT_EQ(0, handler.idle_times());
}
