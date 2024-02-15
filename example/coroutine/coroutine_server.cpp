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


// A server to receive EchoRequest and send back EchoResponse.

#include <gflags/gflags.h>
#include <melon/utility/logging.h>
#include <melon/rpc/server.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/coroutine.h>
#include "echo.pb.h"

DEFINE_int32(port, 8000, "TCP Port of this server");
DEFINE_int32(sleep_us, 1000000, "Server sleep us");
DEFINE_bool(enable_coroutine, true, "Enable coroutine");

using melon::experimental::Awaitable;
using melon::experimental::AwaitableDone;
using melon::experimental::Coroutine;

namespace example {
class EchoServiceImpl : public EchoService {
public:
    EchoServiceImpl() {
        melon::ChannelOptions options;
        options.timeout_ms = FLAGS_sleep_us / 1000 * 2 + 100;
        options.max_retry = 0;
        CHECK(_channel.Init(mutil::EndPoint(mutil::IP_ANY, FLAGS_port), &options) == 0);
    }

    virtual ~EchoServiceImpl() {}

    void Echo(google::protobuf::RpcController* cntl_base,
              const EchoRequest* request,
              EchoResponse* response,
              google::protobuf::Closure* done) override {
        // melon::Controller* cntl =
        //     static_cast<melon::Controller*>(cntl_base);

        if (FLAGS_enable_coroutine) {
            Coroutine(EchoAsync(request, response, done), true);
        } else {
            melon::ClosureGuard done_guard(done);
            fiber_usleep(FLAGS_sleep_us);
            response->set_message(request->message());
        }
    }

    Awaitable<void> EchoAsync(const EchoRequest* request,
               EchoResponse* response,
               google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);
        co_await Coroutine::usleep(FLAGS_sleep_us);
        response->set_message(request->message());
    }

    void Proxy(google::protobuf::RpcController* cntl_base,
               const EchoRequest* request,
               EchoResponse* response,
               google::protobuf::Closure* done) override {
        // melon::Controller* cntl =
        //     static_cast<melon::Controller*>(cntl_base);

        if (FLAGS_enable_coroutine) {
            Coroutine(ProxyAsync(request, response, done), true);
        } else {
            melon::ClosureGuard done_guard(done);
            EchoService_Stub stub(&_channel);
            melon::Controller cntl;
            stub.Echo(&cntl, request, response, NULL);
            if (cntl.Failed()) {
                response->set_message(cntl.ErrorText());
            }
        }
    }

    Awaitable<void> ProxyAsync(const EchoRequest* request,
                    EchoResponse* response,
                    google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);
        EchoService_Stub stub(&_channel);
        melon::Controller cntl;
        AwaitableDone done2;
        stub.Echo(&cntl, request, response, &done2);
        co_await done2.awaitable();
        if (cntl.Failed()) {
            response->set_message(cntl.ErrorText());
        }
    }    

private:
    melon::Channel _channel;
};
}  // namespace example

int main(int argc, char* argv[]) {
    fiber_setconcurrency(FIBER_MIN_CONCURRENCY);

    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_enable_coroutine) {
        GFLAGS_NS::SetCommandLineOption("usercode_in_coroutine", "true");
    }

    // Generally you only need one Server.
    melon::Server server;

    // Instance of your service.
    example::EchoServiceImpl echo_service_impl;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use melon::SERVER_OWNS_SERVICE.
    if (server.AddService(&echo_service_impl, 
                          melon::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    // Start the server.
    melon::ServerOptions options;
    options.num_threads = FIBER_MIN_CONCURRENCY;
    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}