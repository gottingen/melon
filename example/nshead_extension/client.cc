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

// A client sending requests to server every 1 second.

#include <gflags/gflags.h>

#include "melon/log/logging.h"
#include "melon/times/time.h"
#include <melon/rpc/channel.h>
#include <melon/rpc/nshead_message.h>
#include <melon/metrics/all.h>

melon::LatencyRecorder g_latency_recorder("client");

DEFINE_string(server, "0.0.0.0:8010", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);
    
    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    melon::rpc::Channel channel;
    
    // Initialize the channel, nullptr means using default options.
    melon::rpc::ChannelOptions options;
    options.protocol = melon::rpc::PROTOCOL_NSHEAD;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;
    if (channel.Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(), &options) != 0) {
        MELON_LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // Send a request and wait for the response every 1 second.
    int log_id = 0;
    while (!melon::rpc::IsAskedToQuit()) {
        melon::rpc::NsheadMessage request;
        melon::rpc::NsheadMessage response;
        melon::rpc::Controller cntl;

        // Append message to `request'
        request.body.append("hello world");

        cntl.set_log_id(log_id ++);  // set by user

        // Because `done'(last parameter) is nullptr, this function waits until
        // the response comes back or error occurs(including timedout).
        channel.CallMethod(nullptr, &cntl, &request, &response, nullptr);
        if (cntl.Failed()) {
            MELON_LOG(ERROR) << "Fail to send nshead request, " << cntl.ErrorText();
            sleep(1); // Remove this sleep in production code.
        } else {
            g_latency_recorder << cntl.latency_us();
        }
        MELON_LOG_EVERY_SECOND(INFO)
            << "Sending nshead requests at qps=" << g_latency_recorder.qps(1)
            << " latency=" << g_latency_recorder.latency(1);
    }

    MELON_LOG(INFO) << "EchoClient is going to quit";
    return 0;
}
