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


// - Access pb services via HTTP
//   ./http_client http://www.foo.com:8765/EchoService/Echo -d '{"message":"hello"}'
// - Access builtin services
//   ./http_client http://www.foo.com:8765/vars/rpc_server*
// - Access www.foo.com
//   ./http_client www.foo.com

#include <turbo/flags/flag.h>
#include <turbo/flags/declare.h>
#include <turbo/log/logging.h>
#include <melon/rpc/channel.h>

TURBO_FLAG(std::string, d, "", "POST this data to the http server");
TURBO_FLAG(std::string, load_balancer, "", "The algorithm for load balancing");
TURBO_FLAG(int32_t, timeout_ms, 2000, "RPC timeout in milliseconds");
TURBO_FLAG(int32_t, max_retry, 3, "Max retries(not including the first RPC)"); 
TURBO_FLAG(std::string, protocol, "http", "Client-side protocol");
TURBO_DECLARE_FLAG(bool, http_verbose);

int main(int argc, char* argv[]) {

    if (argc != 2) {
        LOG(ERROR) << "Usage: ./http_client \"http(s)://www.foo.com\"";
        return -1;
    }
    char* url = argv[1];
    
    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    melon::Channel channel;
    melon::ChannelOptions options;
    options.protocol = turbo::get_flag(FLAGS_protocol);
    options.timeout_ms = turbo::get_flag(FLAGS_timeout_ms)/*milliseconds*/;
    options.max_retry = turbo::get_flag(FLAGS_max_retry);

    // Initialize the channel, nullptr means using default options. 
    // options, see `melon/rpc/channel.h'.
    if (channel.Init(url, turbo::get_flag(FLAGS_load_balancer).c_str(), &options) != 0) {
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // We will receive response synchronously, safe to put variables
    // on stack.
    melon::Controller cntl;

    cntl.http_request().uri() = url;
    if (!turbo::get_flag(FLAGS_d).empty()) {
        cntl.http_request().set_method(melon::HTTP_METHOD_POST);
        cntl.request_attachment().append(turbo::get_flag(FLAGS_d));
    }

    // Because `done'(last parameter) is nullptr, this function waits until
    // the response comes back or error occurs(including timedout).
    channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
    if (cntl.Failed()) {
        std::cerr << cntl.ErrorText() << std::endl;
        return -1;
    }
    // If -http_verbose is on, melon already prints the response to stderr.
    if (!turbo::get_flag(FLAGS_http_verbose)) {
        std::cout << cntl.response_attachment() << std::endl;
    }
    return 0;
}
