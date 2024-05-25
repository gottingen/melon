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


// A client sending requests to server every 1 second using grpc.

#include <gflags/gflags.h>
#include <melon/utility/logging.h>
#include <melon/utility/time.h>
#include <melon/rpc/channel.h>
#include "helloworld.pb.h"

DEFINE_string(protocol, "h2:grpc", "Protocol type. Defined in melon/rpc/options.proto");
DEFINE_string(server, "0.0.0.0:50051", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 
DEFINE_int32(interval_ms, 1000, "Milliseconds between consecutive requests");
DEFINE_bool(gzip, false, "compress body using gzip");

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_gzip) {
        google::SetCommandLineOption("http_body_compress_threshold", 0);
    }
    
    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    melon::Channel channel;
    
    // Initialize the channel, NULL means using default options.
    melon::ChannelOptions options;
    options.protocol = FLAGS_protocol;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;
    if (channel.Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(), &options) != 0) {
        MLOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // Normally, you should not call a Channel directly, but instead construct
    // a stub Service wrapping it. stub can be shared by all threads as well.
    helloworld::Greeter_Stub stub(&channel);

    // Send a request and wait for the response every 1 second.
    while (!melon::IsAskedToQuit()) {
        // We will receive response synchronously, safe to put variables
        // on stack.
        helloworld::HelloRequest request;
        helloworld::HelloReply response;
        melon::Controller cntl;

        request.set_name("grpc_req_from_melon");
        if (FLAGS_gzip) {
            cntl.set_request_compress_type(melon::COMPRESS_TYPE_GZIP);
        }
        // Because `done'(last parameter) is NULL, this function waits until
        // the response comes back or error occurs(including timedout).
        stub.SayHello(&cntl, &request, &response, NULL);
        if (!cntl.Failed()) {
            MLOG(INFO) << "Received response from " << cntl.remote_side()
                << " to " << cntl.local_side()
                << ": " << response.message()
                << " latency=" << cntl.latency_us() << "us";
        } else {
            MLOG(WARNING) << cntl.ErrorText();
        }
        usleep(FLAGS_interval_ms * 1000L);
    }

    return 0;
}
