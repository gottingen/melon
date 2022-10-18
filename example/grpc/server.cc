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

// A server to receive HelloRequest and send back HelloReply

#include <gflags/gflags.h>
#include "melon/log/logging.h"
#include <melon/rpc/server.h>
#include <melon/rpc/restful.h>
#include "helloworld.pb.h"

DEFINE_int32(port, 50051, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state "
             "(waiting for client to close connection before server stops)");
DEFINE_bool(gzip, false, "compress body using gzip");

class GreeterImpl : public helloworld::Greeter {
public:
    GreeterImpl() {};
    virtual ~GreeterImpl() {};
    void SayHello(google::protobuf::RpcController* cntl_base,
                 const helloworld::HelloRequest* req,
                 helloworld::HelloReply* res,
                 google::protobuf::Closure* done) {
        melon::rpc::ClosureGuard done_guard(done);
        melon::rpc::Controller* cntl = static_cast<melon::rpc::Controller*>(cntl_base);
        if (FLAGS_gzip) {
            cntl->set_response_compress_type(melon::rpc::COMPRESS_TYPE_GZIP);
        }
        res->set_message("Hello " + req->name());
    }
};

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // Generally you only need one Server.
    melon::rpc::Server server;

    GreeterImpl http_svc;

    // Add services into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use melon::rpc::SERVER_OWNS_SERVICE.
    if (server.AddService(&http_svc,
                          melon::rpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        MELON_LOG(ERROR) << "Fail to add http_svc";
        return -1;
    }

    // Start the server.
    melon::rpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    if (server.Start(FLAGS_port, &options) != 0) {
        MELON_LOG(ERROR) << "Fail to start HttpServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
