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

// A client sending requests to server asynchronously every 1 second.

#include <gflags/gflags.h>
#include "melon/log/logging.h"
#include "melon/times/time.h"
#include <melon/rpc/channel.h>
#include "echo.pb.h"

DEFINE_bool(send_attachment, true, "Carry attachment along with requests");
DEFINE_string(protocol, "baidu_std", "Protocol type. Defined in melon/rpc/options.proto");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:8003", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 

void HandleEchoResponse(
        melon::rpc::Controller* cntl,
        example::EchoResponse* response) {
    // std::unique_ptr makes sure cntl/response will be deleted before returning.
    std::unique_ptr<melon::rpc::Controller> cntl_guard(cntl);
    std::unique_ptr<example::EchoResponse> response_guard(response);

    if (cntl->Failed()) {
        MELON_LOG(WARNING) << "Fail to send EchoRequest, " << cntl->ErrorText();
        return;
    }
    MELON_LOG(INFO) << "Received response from " << cntl->remote_side()
        << ": " << response->message() << " (attached="
        << cntl->response_attachment() << ")"
        << " latency=" << cntl->latency_us() << "us";
}
                        

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    melon::rpc::Channel channel;

    // Initialize the channel, NULL means using default options.
    melon::rpc::ChannelOptions options;
    options.protocol = FLAGS_protocol;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;
    if (channel.Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(), &options) != 0) {
        MELON_LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // Normally, you should not call a Channel directly, but instead construct
    // a stub Service wrapping it. stub can be shared by all threads as well.
    example::EchoService_Stub stub(&channel);
    
    // Send a request and wait for the response every 1 second.
    int log_id = 0;
    while (!melon::rpc::IsAskedToQuit()) {
        // Since we are sending asynchronous RPC (`done' is not NULL),
        // these objects MUST remain valid until `done' is called.
        // As a result, we allocate these objects on heap
        example::EchoResponse* response = new example::EchoResponse();
        melon::rpc::Controller* cntl = new melon::rpc::Controller();

        // Notice that you don't have to new request, which can be modified
        // or destroyed just after stub.Echo is called.
        example::EchoRequest request;
        request.set_message("hello world");

        cntl->set_log_id(log_id ++);  // set by user
        if (FLAGS_send_attachment) {
            // Set attachment which is wired to network directly instead of 
            // being serialized into protobuf messages.
            cntl->request_attachment().append("foo");
        }

        // We use protobuf utility `NewCallback' to create a closure object
        // that will call our callback `HandleEchoResponse'. This closure
        // will automatically delete itself after being called once
        google::protobuf::Closure* done = melon::rpc::NewCallback(
            &HandleEchoResponse, cntl, response);
        stub.Echo(cntl, &request, response, done);

        // This is an asynchronous RPC, so we can only fetch the result
        // inside the callback
        sleep(1);
    }

    MELON_LOG(INFO) << "EchoClient is going to quit";
    return 0;
}
