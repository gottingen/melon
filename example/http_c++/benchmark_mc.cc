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

// Benchmark http-server by multiple threads.

#include <gflags/gflags.h>
#include <melon/bthread/bthread.h>
#include <melon/butil/logging.h>
#include <melon/mc/http.h>
#include <melon/rpc/server.h>
#include <melon/var/var.h>

DEFINE_string(data, "", "POST this data to the http server");
DEFINE_int32(thread_num, 50, "Number of threads to send requests");
DEFINE_bool(use_bthread, false, "Use bthread to send requests");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(url, "0.0.0.0:8010/HttpService/Echo", "url of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 
DEFINE_bool(dont_fail, false, "Print fatal when some call failed");
DEFINE_int32(dummy_port, -1, "Launch dummy server at this port");
DEFINE_string(protocol, "http", "Client-side protocol");

melon::var::LatencyRecorder g_latency_recorder("client");

static void* sender(void* arg) {
    melon::HttpChannelPool* channel = static_cast<melon::HttpChannelPool*>(arg);

    while (!melon::IsAskedToQuit()) {

        melon::Http http;
        http.set_channel(channel->get_channel())
        .set_timeout(FLAGS_timeout_ms)
        .set_max_retry(FLAGS_max_retry)
        .set_uri(FLAGS_url);

        bool ok;
        if (!FLAGS_data.empty()) {
           ok =  http.post(FLAGS_data);
        } else {
            ok = http.get();
        }

        if (ok) {
            g_latency_recorder << http.latency_us();
        } else {
            CHECK(melon::IsAskedToQuit() || !FLAGS_dont_fail)
                << "error=" << http.error_message() << " latency=" << http.latency_us();
            // We can't connect to the server, sleep a while. Notice that this
            // is a specific sleeping to prevent this thread from spinning too
            // fast. You should continue the business logic in a production 
            // server rather than sleeping.
            bthread_usleep(100000);
        }
    }
    return nullptr;
}

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);


    melon::ChannelOptions options;
    options.protocol = FLAGS_protocol;
    options.connection_type = FLAGS_connection_type;
    melon::HttpChannelPool channel_pool;
    auto ret = channel_pool.initialize(FLAGS_url, FLAGS_thread_num, FLAGS_load_balancer, false, &options);
    
    // Initialize the channel, nullptr means using default options. 
    // options, see `melon/rpc/channel.h'.
    if (ret != 0) {
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    std::vector<bthread_t> bids;
    std::vector<pthread_t> pids;
    if (!FLAGS_use_bthread) {
        pids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (pthread_create(&pids[i], nullptr, sender, &channel_pool) != 0) {
                LOG(ERROR) << "Fail to create pthread";
                return -1;
            }
        }
    } else {
        bids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (bthread_start_background(
                    &bids[i], nullptr, sender, &channel_pool) != 0) {
                LOG(ERROR) << "Fail to create bthread";
                return -1;
            }
        }
    }

    if (FLAGS_dummy_port >= 0) {
        melon::StartDummyServerAt(FLAGS_dummy_port);
    }

    while (!melon::IsAskedToQuit()) {
        sleep(1);
        LOG(INFO) << "Sending " << FLAGS_protocol << " requests at qps=" 
                  << g_latency_recorder.qps(1)
                  << " latency=" << g_latency_recorder.latency(1);
    }

    LOG(INFO) << "benchmark_http is going to quit";
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        if (!FLAGS_use_bthread) {
            pthread_join(pids[i], nullptr);
        } else {
            bthread_join(bids[i], nullptr);
        }
    }

    return 0;
}
