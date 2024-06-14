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
// Created by jeff on 24-6-14.
//


#include <gflags/gflags.h>
#include <turbo/log/logging.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/restful_service.h>

DEFINE_string(d, "", "POST this data to the http server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 2000, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
DEFINE_string(protocol, "http", "Client-side protocol");

namespace melon {
    DECLARE_bool(http_verbose);
}

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    if (argc != 2) {
        LOG(ERROR) << "Usage: ./restful_client \"http(s)://www.foo.com\"";
        return -1;
    }
    char* url = argv[1];

    // A Channel represents a communication line to a Server. Notice that
    // Channel is thread-safe and can be shared by all threads in your program.
    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = FLAGS_protocol;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;

    // Initialize the channel, nullptr means using default options.
    // options, see `melon/rpc/channel.h'.
    if (channel.Init(url, FLAGS_load_balancer.c_str(), &options) != 0) {
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // We will receive response synchronously, safe to put variables
    // on stack.
    melon::RestfulClient client;
    client.set_channel(&channel);
    auto _rs = client.create_request();
    if (!_rs.ok()) {
        LOG(ERROR) << "Fail to create request: " << _rs.status();
        return -1;
    }
    auto req = _rs.value();
    req.set_uri(url);
    if (!FLAGS_d.empty()) {
        req.set_method(melon::HTTP_METHOD_POST);
        req.set_body(FLAGS_d);
    } else {
        req.set_method(melon::HTTP_METHOD_GET);
    }

    auto res = client.do_request(req);

    if (res.failed()) {
        std::cerr << res.failed_reason() << std::endl;
        return -1;
    }
    // If -http_verbose is on, melon already prints the response to stderr.
    if (!melon::FLAGS_http_verbose) {
        std::cout << res.body() << std::endl;
    }
    return 0;
}
