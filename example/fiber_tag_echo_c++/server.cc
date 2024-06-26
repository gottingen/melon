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


// A server to receive EchoRequest and send back EchoResponse.

#include <gflags/gflags.h>
#include <turbo/log/logging.h>
#include <melon/rpc/server.h>
#include <melon/fiber/unstable.h>
#include "echo.pb.h"

DEFINE_bool(echo_attachment, true, "Echo attachment as well");
DEFINE_int32(port1, 8002, "TCP Port of this server");
DEFINE_int32(port2, 8003, "TCP Port of this server");
DEFINE_int32(tag1, 0, "Server1 tag");
DEFINE_int32(tag2, 1, "Server2 tag");
DEFINE_int32(tag3, 2, "Background task tag");
DEFINE_int32(idle_timeout_s, -1,
             "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(max_concurrency, 0, "Limit of request processing in parallel");
DEFINE_int32(internal_port1, -1, "Only allow builtin services at this port");
DEFINE_int32(internal_port2, -1, "Only allow builtin services at this port");

namespace example {
// Your implementation of EchoService
class EchoServiceImpl : public EchoService {
public:
    EchoServiceImpl() {}
    ~EchoServiceImpl() {}
    void Echo(google::protobuf::RpcController* cntl_base, const EchoRequest* request,
              EchoResponse* response, google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);
        melon::Controller* cntl = static_cast<melon::Controller*>(cntl_base);

        // Echo request and its attachment
        response->set_message(request->message());
        if (FLAGS_echo_attachment) {
            cntl->response_attachment().append(cntl->request_attachment());
        }
    }
};
}  // namespace example

DEFINE_bool(h, false, "print help information");

static void my_tagged_worker_start_fn(fiber_tag_t tag) {
    LOG(INFO) << "run tagged worker start function tag=" << tag;
}

static void* my_background_task(void*) {
    LOG(INFO) << "run background task tag=" << fiber_self_tag();
    fiber_usleep(1000000UL);
    return NULL;
}

int main(int argc, char* argv[]) {
    std::string help_str = "dummy help infomation";
    google::SetUsageMessage(help_str);

    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_h) {
        fprintf(stderr, "%s\n%s\n%s", help_str.c_str(), help_str.c_str(), help_str.c_str());
        return 0;
    }

    // Set tagged worker function
    fiber_set_tagged_worker_startfn(my_tagged_worker_start_fn);

    // Generally you only need one Server.
    melon::Server server1;

    // Instance of your service.
    example::EchoServiceImpl echo_service_impl1;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use melon::SERVER_OWNS_SERVICE.
    if (server1.AddService(&echo_service_impl1, melon::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    // Start the server.
    melon::ServerOptions options1;
    options1.idle_timeout_sec = FLAGS_idle_timeout_s;
    options1.max_concurrency = FLAGS_max_concurrency;
    options1.internal_port = FLAGS_internal_port1;
    options1.fiber_tag = FLAGS_tag1;
    if (server1.Start(FLAGS_port1, &options1) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Generally you only need one Server.
    melon::Server server2;

    // Instance of your service.
    example::EchoServiceImpl echo_service_impl2;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use melon::SERVER_OWNS_SERVICE.
    if (server2.AddService(&echo_service_impl2, melon::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    // Start the server.
    melon::ServerOptions options2;
    options2.idle_timeout_sec = FLAGS_idle_timeout_s;
    options2.max_concurrency = FLAGS_max_concurrency;
    options2.internal_port = FLAGS_internal_port2;
    options2.fiber_tag = FLAGS_tag2;
    if (server2.Start(FLAGS_port2, &options2) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Start backgroup task
    fiber_t tid;
    fiber_attr_t attr = FIBER_ATTR_NORMAL;
    attr.tag = FLAGS_tag3;
    fiber_start_background(&tid, &attr, my_background_task, nullptr);

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server1.RunUntilAskedToQuit();
    server2.RunUntilAskedToQuit();

    return 0;
}
