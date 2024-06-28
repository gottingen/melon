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


// Benchmark http-server by multiple threads.

#include <turbo/flags/flag.h>
#include <melon/fiber/fiber.h>
#include <turbo/log/logging.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/server.h>
#include <melon/var/var.h>

TURBO_FLAG(std::string, data, "", "POST this data to the http server");
TURBO_FLAG(int32_t, thread_num, 50, "Number of threads to send requests");
TURBO_FLAG(bool, use_fiber, false, "Use fiber to send requests");
TURBO_FLAG(std::string, connection_type, "", "Connection type. Available values: single, pooled, short");
TURBO_FLAG(std::string, url, "0.0.0.0:8038/HttpService/Echo", "url of server");
TURBO_FLAG(std::string, load_balancer, "", "The algorithm for load balancing");
TURBO_FLAG(int32_t, timeout_ms, 100, "RPC timeout in milliseconds");
TURBO_FLAG(int32_t, max_retry, 3, "Max retries(not including the first RPC)"); 
TURBO_FLAG(bool, dont_fail, false, "Print fatal when some call failed");
TURBO_FLAG(int32_t, dummy_port, -1, "Launch dummy server at this port");
TURBO_FLAG(std::string, protocol, "http", "Client-side protocol");

melon::var::LatencyRecorder g_latency_recorder("client");

static void* sender(void* arg) {
    melon::Channel* channel = static_cast<melon::Channel*>(arg);

    while (!melon::IsAskedToQuit()) {
        // We will receive response synchronously, safe to put variables
        // on stack.
        melon::Controller cntl;

        cntl.set_timeout_ms(turbo::get_flag(FLAGS_timeout_ms)/*milliseconds*/);
        cntl.set_max_retry(turbo::get_flag(FLAGS_max_retry));
        cntl.http_request().uri() = turbo::get_flag(FLAGS_url);
        if (!turbo::get_flag(FLAGS_data).empty()) {
            cntl.http_request().set_method(melon::HTTP_METHOD_POST);
            cntl.request_attachment().append(turbo::get_flag(FLAGS_data));
        }

        // Because `done'(last parameter) is nullptr, this function waits until
        // the response comes back or error occurs(including timedout).
        channel->CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
        if (!cntl.Failed()) {
            g_latency_recorder << cntl.latency_us();
        } else {
            CHECK(melon::IsAskedToQuit() || !turbo::get_flag(FLAGS_dont_fail))
                << "error=" << cntl.ErrorText() << " latency=" << cntl.latency_us();
            // We can't connect to the server, sleep a while. Notice that this
            // is a specific sleeping to prevent this thread from spinning too
            // fast. You should continue the business logic in a production 
            // server rather than sleeping.
            fiber_usleep(100000);
        }
    }
    return nullptr;
}

int main(int argc, char* argv[]) {

    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = turbo::get_flag(FLAGS_protocol);
    options.connection_type = turbo::get_flag(FLAGS_connection_type);
    
    // Initialize the channel, nullptr means using default options. 
    // options, see `melon/rpc/channel.h'.
    if (channel.Init(turbo::get_flag(FLAGS_url).c_str(), turbo::get_flag(FLAGS_load_balancer).c_str(), &options) != 0) {
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    std::vector<fiber_t> bids;
    std::vector<pthread_t> pids;
    if (!turbo::get_flag(FLAGS_use_fiber)) {
        pids.resize(turbo::get_flag(FLAGS_thread_num));
        for (int i = 0; i < turbo::get_flag(FLAGS_thread_num); ++i) {
            if (pthread_create(&pids[i], nullptr, sender, &channel) != 0) {
                LOG(ERROR) << "Fail to create pthread";
                return -1;
            }
        }
    } else {
        bids.resize(turbo::get_flag(FLAGS_thread_num));
        for (int i = 0; i < turbo::get_flag(FLAGS_thread_num); ++i) {
            if (fiber_start_background(
                    &bids[i], nullptr, sender, &channel) != 0) {
                LOG(ERROR) << "Fail to create fiber";
                return -1;
            }
        }
    }

    if (turbo::get_flag(FLAGS_dummy_port) >= 0) {
        melon::StartDummyServerAt(turbo::get_flag(FLAGS_dummy_port));
    }

    while (!melon::IsAskedToQuit()) {
        sleep(1);
        LOG(INFO) << "Sending " << turbo::get_flag(FLAGS_protocol) << " requests at qps="
                  << g_latency_recorder.qps(1)
                  << " latency=" << g_latency_recorder.latency(1);
    }

    LOG(INFO) << "benchmark_http is going to quit";
    for (int i = 0; i < turbo::get_flag(FLAGS_thread_num); ++i) {
        if (!turbo::get_flag(FLAGS_use_fiber)) {
            pthread_join(pids[i], nullptr);
        } else {
            fiber_join(bids[i], nullptr);
        }
    }

    return 0;
}
