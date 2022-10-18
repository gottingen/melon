
#include <gflags/gflags.h>
#include "melon/log/logging.h"
#include <melon/rpc/server.h>
#include "echo.pb.h"

DEFINE_bool(echo_attachment, true, "Echo attachment as well");
DEFINE_int32(port, 8002, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state "
             "(waiting for client to close connection before server stops)");
DEFINE_int32(max_concurrency, 0, "Limit of request processing in parallel");

// Your implementation of example::EchoService
class EchoServiceImpl : public example::EchoService {
public:
    EchoServiceImpl() {}
    ~EchoServiceImpl() {};
    void Echo(google::protobuf::RpcController* cntl_base,
              const example::EchoRequest* request,
              example::EchoResponse* response,
              google::protobuf::Closure* done) {
        melon::rpc::ClosureGuard done_guard(done);
        melon::rpc::Controller* cntl =
            static_cast<melon::rpc::Controller*>(cntl_base);

        // Echo request and its attachment
        response->set_value(request->value());
        if (FLAGS_echo_attachment) {
            cntl->response_attachment().append(cntl->request_attachment());
        }
    }
};

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // Generally you only need one Server.
    melon::rpc::Server server;

    // Instance of your service.
    EchoServiceImpl echo_service_impl;

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
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    options.max_concurrency = FLAGS_max_concurrency;
    if (server.Start(FLAGS_port, &options) != 0) {
        MELON_LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
