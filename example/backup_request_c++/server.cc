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


// A server sleeping for even-th requests to trigger backup request of client.

#include <gflags/gflags.h>
#include <melon/utility/logging.h>
#include <melon/rpc/server.h>
#include "echo.pb.h"

DEFINE_bool(echo_attachment, true, "Echo attachment as well");
DEFINE_int32(port, 8000, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(sleep_ms, 20, "Sleep so many milliseconds on even-th requests");

// Your implementation of example::EchoService
// Notice that implementing melon::Describable grants the ability to put
// additional information in /status.
namespace example {
class SleepyEchoService : public EchoService
                        , public melon::Describable {
public:
    SleepyEchoService() : _count(0) {}
    virtual ~SleepyEchoService() {}
    virtual void Echo(google::protobuf::RpcController* cntl_base,
                      const EchoRequest* request,
                      EchoResponse* response,
                      google::protobuf::Closure* done) {
        // This object helps you to call done->Run() in RAII style. If you need
        // to process the request asynchronously, pass done_guard.release().
        melon::ClosureGuard done_guard(done);

        melon::Controller* cntl =
            static_cast<melon::Controller*>(cntl_base);

        // The purpose of following logs is to help you to understand
        // how clients interact with servers more intuitively. You should 
        // remove these logs in performance-sensitive servers.
        // The noflush prevents the log from being flushed immediately.
        MLOG(INFO) << "Received request[index=" << request->index()
                  << "] from " << cntl->remote_side() 
                  << " to " << cntl->local_side() << noflush;
        // Sleep a while for 0th, 2nd, 4th, 6th ... requests to trigger backup request
        // at client-side.
        bool do_sleep = (_count.fetch_add(1, mutil::memory_order_relaxed) % 2 == 0);
        if (do_sleep) {
            MLOG(INFO) << ", sleep " << FLAGS_sleep_ms
                      << " ms to trigger backup request" << noflush;
        }
        MLOG(INFO);

        // Fill response.
        response->set_index(request->index());

        if (do_sleep) {
            fiber_usleep(FLAGS_sleep_ms * 1000);
        }
    }
private:
    mutil::atomic<int> _count;
};
}  // namespace example

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // Generally you only need one Server.
    melon::Server server;

    // Instance of your service.
    example::SleepyEchoService echo_service_impl;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use melon::SERVER_OWNS_SERVICE.
    if (server.AddService(&echo_service_impl, 
                          melon::SERVER_DOESNT_OWN_SERVICE) != 0) {
        MLOG(ERROR) << "Fail to add service";
        return -1;
    }

    // Start the server.
    melon::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    if (server.Start(FLAGS_port, &options) != 0) {
        MLOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
