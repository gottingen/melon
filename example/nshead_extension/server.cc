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

// A server to receive EchoRequest and send back EchoResponse.

#include <gflags/gflags.h>
#include "melon/log/logging.h"
#include <melon/rpc/server.h>
#include <melon/rpc/nshead_service.h>

DEFINE_int32(port, 8010, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(max_concurrency, 0, "Limit of request processing in parallel");

// Adapt your own nshead-based protocol to use melon
class MyNsheadProtocol : public melon::rpc::NsheadService {
public:
    void ProcessNsheadRequest(const melon::rpc::Server&,
                              melon::rpc::Controller* cntl,
                              const melon::rpc::NsheadMessage& request,
                              melon::rpc::NsheadMessage* response,
                              melon::rpc::NsheadClosure* done) {
        // This object helps you to call done->Run() in RAII style. If you need
        // to process the request asynchronously, pass done_guard.release().
        melon::rpc::ClosureGuard done_guard(done);

        if (cntl->Failed()) {
            // NOTE: You can send back a response containing error information
            // back to client instead of closing the connection.
            cntl->CloseConnection("Close connection due to previous error");
            return;
        }
        *response = request; // Just echo the request to client
    }
};

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    melon::rpc::Server server;
    melon::rpc::ServerOptions options;
    options.nshead_service = new MyNsheadProtocol;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    options.max_concurrency = FLAGS_max_concurrency;

    // Start the server.
    if (server.Start(FLAGS_port, &options) != 0) {
        MELON_LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
