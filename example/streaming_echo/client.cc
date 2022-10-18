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

// A client sending requests to server in batch every 1 second.

#include <gflags/gflags.h>
#include "melon/log/logging.h"
#include <melon/rpc/channel.h>
#include <melon/rpc/stream.h>
#include "echo.pb.h"

DEFINE_bool(send_attachment, true, "Carry attachment along with requests");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:8001", "IP Address of server");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    melon::rpc::Channel channel;
        
    // Initialize the channel, NULL means using default options. 
    melon::rpc::ChannelOptions options;
    options.protocol = melon::rpc::PROTOCOL_BAIDU_STD;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;
    if (channel.Init(FLAGS_server.c_str(), NULL) != 0) {
        MELON_LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // Normally, you should not call a Channel directly, but instead construct
    // a stub Service wrapping it. stub can be shared by all threads as well.
    example::EchoService_Stub stub(&channel);
    melon::rpc::Controller cntl;
    melon::rpc::StreamId stream;
    if (melon::rpc::StreamCreate(&stream, cntl, NULL) != 0) {
        MELON_LOG(ERROR) << "Fail to create stream";
        return -1;
    }
    MELON_LOG(INFO) << "Created Stream=" << stream;
    example::EchoRequest request;
    example::EchoResponse response;
    request.set_message("I'm a RPC to connect stream");
    stub.Echo(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        MELON_LOG(ERROR) << "Fail to connect stream, " << cntl.ErrorText();
        return -1;
    }
    
    while (!melon::rpc::IsAskedToQuit()) {
        melon::cord_buf msg1;
        msg1.append("abcdefghijklmnopqrstuvwxyz");
        MELON_CHECK_EQ(0, melon::rpc::StreamWrite(stream, msg1));
        melon::cord_buf msg2;
        msg2.append("0123456789");
        MELON_CHECK_EQ(0, melon::rpc::StreamWrite(stream, msg2));
        sleep(1);
    }

    MELON_CHECK_EQ(0, melon::rpc::StreamClose(stream));
    MELON_LOG(INFO) << "EchoClient is going to quit";
    return 0;
}
