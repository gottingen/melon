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


// A client sending requests to server in parallel by multiple threads.

#include <gflags/gflags.h>
#include <melon/fiber/fiber.h>
#include <melon/utility/logging.h>
#include <melon/utility/string_printf.h>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <melon/rpc/parallel_channel.h>
#include <melon/rpc/server.h>
#include "echo.pb.h"

DEFINE_int32(thread_num, 50, "Number of threads to send requests");
DEFINE_int32(channel_num, 3, "Number of sub channels");
DEFINE_bool(same_channel, false, "Add the same sub channel multiple times");
DEFINE_bool(use_fiber, false, "Use fiber to send requests");
DEFINE_int32(attachment_size, 0, "Carry so many byte attachment along with requests");
DEFINE_int32(request_size, 16, "Bytes of each request");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(protocol, "melon_std", "Protocol type. Defined in melon/rpc/options.proto");
DEFINE_string(server, "0.0.0.0:8002", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 
DEFINE_bool(dont_fail, false, "Print fatal when some call failed");
DEFINE_int32(dummy_port, -1, "Launch dummy server at this port");

std::string g_request;
std::string g_attachment;
melon::var::LatencyRecorder g_latency_recorder("client");
melon::var::Adder<int> g_error_count("client_error_count");
melon::var::LatencyRecorder* g_sub_channel_latency = NULL;

static void* sender(void* arg) {
    // Normally, you should not call a Channel directly, but instead construct
    // a stub Service wrapping it. stub can be shared by all threads as well.
    example::EchoService_Stub stub(static_cast<google::protobuf::RpcChannel*>(arg));

    int log_id = 0;
    while (!melon::IsAskedToQuit()) {
        // We will receive response synchronously, safe to put variables
        // on stack.
        example::EchoRequest request;
        example::EchoResponse response;
        melon::Controller cntl;

        request.set_value(log_id++);
        if (!g_attachment.empty()) {
            // Set attachment which is wired to network directly instead of 
            // being serialized into protobuf messages.
            cntl.request_attachment().append(g_attachment);
        }

        // Because `done'(last parameter) is NULL, this function waits until
        // the response comes back or error occurs(including timedout).
        stub.Echo(&cntl, &request, &response, NULL);
        if (!cntl.Failed()) {
            g_latency_recorder << cntl.latency_us();
            for (int i = 0; i < cntl.sub_count(); ++i) {
                if (cntl.sub(i) && !cntl.sub(i)->Failed()) {
                    g_sub_channel_latency[i] << cntl.sub(i)->latency_us();
                }
            }
        } else {
            g_error_count << 1;
            MCHECK(melon::IsAskedToQuit() || !FLAGS_dont_fail)
                << "error=" << cntl.ErrorText() << " latency=" << cntl.latency_us();
            // We can't connect to the server, sleep a while. Notice that this
            // is a specific sleeping to prevent this thread from spinning too
            // fast. You should continue the business logic in a production 
            // server rather than sleeping.
            fiber_usleep(50000);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    melon::ParallelChannel channel;
    melon::ParallelChannelOptions pchan_options;
    pchan_options.timeout_ms = FLAGS_timeout_ms;
    if (channel.Init(&pchan_options) != 0) {
        MLOG(ERROR) << "Fail to init ParallelChannel";
        return -1;
    }

    melon::ChannelOptions sub_options;
    sub_options.protocol = FLAGS_protocol;
    sub_options.connection_type = FLAGS_connection_type;
    sub_options.max_retry = FLAGS_max_retry;
    // Setting sub_options.timeout_ms does not work because timeout of sub 
    // channels are disabled in ParallelChannel.

    if (FLAGS_same_channel) {
        // For melon >= 1.0.155.31351, a sub channel can be added into
        // a ParallelChannel more than once.
        melon::Channel* sub_channel = new melon::Channel;
        // Initialize the channel, NULL means using default options. 
        // options, see `melon/rpc/channel.h'.
        if (sub_channel->Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(), &sub_options) != 0) {
            MLOG(ERROR) << "Fail to initialize sub_channel";
            return -1;
        }
        for (int i = 0; i < FLAGS_channel_num; ++i) {
            if (channel.AddChannel(sub_channel, melon::OWNS_CHANNEL,
                                   NULL, NULL) != 0) {
                MLOG(ERROR) << "Fail to AddChannel, i=" << i;
                return -1;
            }
        }
    } else {
        for (int i = 0; i < FLAGS_channel_num; ++i) {
            melon::Channel* sub_channel = new melon::Channel;
            // Initialize the channel, NULL means using default options. 
            // options, see `melon/rpc/channel.h'.
            if (sub_channel->Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(), &sub_options) != 0) {
                MLOG(ERROR) << "Fail to initialize sub_channel[" << i << "]";
                return -1;
            }
            if (channel.AddChannel(sub_channel, melon::OWNS_CHANNEL,
                                   NULL, NULL) != 0) {
                MLOG(ERROR) << "Fail to AddChannel, i=" << i;
                return -1;
            }
        }
    }

    // Initialize var for sub channel
    g_sub_channel_latency = new melon::var::LatencyRecorder[FLAGS_channel_num];
    for (int i = 0; i < FLAGS_channel_num; ++i) {
        std::string name;
        mutil::string_printf(&name, "client_sub_%d", i);
        g_sub_channel_latency[i].expose(name);
    }

    if (FLAGS_attachment_size > 0) {
        g_attachment.resize(FLAGS_attachment_size, 'a');
    }
    if (FLAGS_request_size <= 0) {
        MLOG(ERROR) << "Bad request_size=" << FLAGS_request_size;
        return -1;
    }
    g_request.resize(FLAGS_request_size, 'r');

    if (FLAGS_dummy_port >= 0) {
        melon::StartDummyServerAt(FLAGS_dummy_port);
    }

    std::vector<fiber_t> bids;
    std::vector<pthread_t> pids;
    if (!FLAGS_use_fiber) {
        pids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (pthread_create(&pids[i], NULL, sender, &channel) != 0) {
                MLOG(ERROR) << "Fail to create pthread";
                return -1;
            }
        }
    } else {
        bids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (fiber_start_background(
                    &bids[i], NULL, sender, &channel) != 0) {
                MLOG(ERROR) << "Fail to create fiber";
                return -1;
            }
        }
    }

    while (!melon::IsAskedToQuit()) {
        sleep(1);
        MLOG(INFO) << "Sending EchoRequest at qps=" << g_latency_recorder.qps(1)
                  << " latency=" << g_latency_recorder.latency(1) << noflush;
        for (int i = 0; i < FLAGS_channel_num; ++i) {
            MLOG(INFO) << " latency_" << i << "="
                      << g_sub_channel_latency[i].latency(1)
                      << noflush;
        }
        MLOG(INFO);
    }
    
    MLOG(INFO) << "EchoClient is going to quit";
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        if (!FLAGS_use_fiber) {
            pthread_join(pids[i], NULL);
        } else {
            fiber_join(bids[i], NULL);
        }
    }

    return 0;
}