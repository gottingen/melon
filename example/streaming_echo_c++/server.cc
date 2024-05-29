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
#include "echo.pb.h"
#include <melon/rpc/stream.h>

DEFINE_bool(send_attachment, true, "Carry attachment along with response");
DEFINE_int32(port, 8001, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");

class StreamReceiver : public melon::StreamInputHandler {
public:
    virtual int on_received_messages(melon::StreamId id,
                                     mutil::IOBuf *const messages[],
                                     size_t size) {
        std::ostringstream os;
        for (size_t i = 0; i < size; ++i) {
            os << "msg[" << i << "]=" << *messages[i];
        }
        LOG(INFO) << "Received from Stream=" << id << ": " << os.str();
        return 0;
    }
    virtual void on_idle_timeout(melon::StreamId id) {
        LOG(INFO) << "Stream=" << id << " has no data transmission for a while";
    }
    virtual void on_closed(melon::StreamId id) {
        LOG(INFO) << "Stream=" << id << " is closed";
    }

};

// Your implementation of example::EchoService
class StreamingEchoService : public example::EchoService {
public:
    StreamingEchoService() : _sd(melon::INVALID_STREAM_ID) {}
    virtual ~StreamingEchoService() {
        melon::StreamClose(_sd);
    };
    virtual void Echo(google::protobuf::RpcController* controller,
                      const example::EchoRequest* /*request*/,
                      example::EchoResponse* response,
                      google::protobuf::Closure* done) {
        // This object helps you to call done->Run() in RAII style. If you need
        // to process the request asynchronously, pass done_guard.release().
        melon::ClosureGuard done_guard(done);

        melon::Controller* cntl =
            static_cast<melon::Controller*>(controller);
        melon::StreamOptions stream_options;
        stream_options.handler = &_receiver;
        if (melon::StreamAccept(&_sd, *cntl, &stream_options) != 0) {
            cntl->SetFailed("Fail to accept stream");
            return;
        }
        response->set_message("Accepted stream");
    }

private:
    StreamReceiver _receiver;
    melon::StreamId _sd;
};

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // Generally you only need one Server.
    melon::Server server;

    // Instance of your service.
    StreamingEchoService echo_service_impl;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use melon::SERVER_OWNS_SERVICE.
    if (server.AddService(&echo_service_impl, 
                          melon::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    // Start the server. 
    melon::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
