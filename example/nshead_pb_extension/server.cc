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

#include <google/protobuf/descriptor.h>
#include <gflags/gflags.h>
#include "melon/log/logging.h"
#include <melon/rpc/server.h>
#include <melon/rpc/nshead_pb_service_adaptor.h>
#include "echo.pb.h"

DEFINE_int32(port, 8010, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");

// Your implementation of example::EchoService
namespace example {
class EchoServiceImpl : public EchoService {
public:
    EchoServiceImpl() {};
    virtual ~EchoServiceImpl() {};
    virtual void Echo(google::protobuf::RpcController*,
                      const EchoRequest* request,
                      EchoResponse* response,
                      google::protobuf::Closure* done) {
        // This object helps you to call done->Run() in RAII style. If you need
        // to process the request asynchronously, pass done_guard.release().
        melon::rpc::ClosureGuard done_guard(done);

        // Echo response.
        response->set_message(request->message());
    }
};
}  // namespace example

// Adapt your own nshead-based protocol to use pbrpc interface
class MyNsheadProtocol : public melon::rpc::NsheadPbServiceAdaptor {
public:
    void ParseNsheadMeta(
        const melon::rpc::Server&, const melon::rpc::NsheadMessage&,
        melon::rpc::Controller*,
        melon::rpc::NsheadMeta* out_meta) const {
        // Always use EchoService::Echo
        const google::protobuf::ServiceDescriptor* svc =
                example::EchoService::descriptor();
        out_meta->set_full_method_name(svc->method(0)->full_name());
    }

    void ParseRequestFromCordBuf(const melon::rpc::NsheadMeta&,
                               const melon::rpc::NsheadMessage& raw_req,
                               melon::rpc::Controller* cntl,
                               google::protobuf::Message* pb_req) const {
        // `req' MUST be EchoRequest here since we have only one RPCMethod
        example::EchoRequest* echo_req =
                dynamic_cast<example::EchoRequest*>(pb_req);
        if (!echo_req) {
            cntl->SetFailed(melon::rpc::EREQUEST, "Fail to parse request");
            return;
        }
        echo_req->set_message(raw_req.body.to_string());
    }

    void SerializeResponseToCordBuf(
        const melon::rpc::NsheadMeta&, melon::rpc::Controller* cntl,
        const google::protobuf::Message* pb_res,
        melon::rpc::NsheadMessage* raw_res) const {
        if (cntl->Failed()) {
            // Can't send failure feedback in this protocol
            cntl->CloseConnection("Close connection due to previous error");
            return;
        }
        // `res' MUST be EchoResponse here since we have only one RPCMethod
        const example::EchoResponse* echo_res =
                dynamic_cast<const example::EchoResponse*>(pb_res);
        if (!echo_res) {
            cntl->CloseConnection("Close connection due to bad response");
            return;
        }
        raw_res->body.append(echo_res->message());
    }
};

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    melon::rpc::Server server;
    example::EchoServiceImpl echo_service_impl;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use melon::rpc::SERVER_OWNS_SERVICE.
    if (server.AddService(&echo_service_impl, 
                          melon::rpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        MELON_LOG(ERROR) << "Fail to add service";
        return -1;
    }

    // Start the server.
    melon::rpc::ServerOptions options;
    options.nshead_service = new MyNsheadProtocol;  // the adaptor
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    if (server.Start(FLAGS_port, &options) != 0) {
        MELON_LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
