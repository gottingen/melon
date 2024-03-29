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


#include <ostream>
#include "melon/rpc/closure_guard.h"        // ClosureGuard
#include "melon/rpc/controller.h"           // Controller
#include "melon/rpc/builtin/common.h"
#include "melon/rpc/builtin/sockets_service.h"
#include "melon/rpc/socket.h"


namespace melon::rpc {

    void SocketsService::default_method(::google::protobuf::RpcController *cntl_base,
                                        const ::melon::rpc::SocketsRequest *,
                                        ::melon::rpc::SocketsResponse *,
                                        ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        cntl->http_response().set_content_type("text/plain");
        melon::cord_buf_builder os;
        const std::string &constraint = cntl->http_request().unresolved_path();

        if (constraint.empty()) {
            os << "# Use /sockets/<SocketId>\n"
               << melon::describe_resources<Socket>() << '\n';
        } else {
            char *endptr = nullptr;
            SocketId sid = strtoull(constraint.c_str(), &endptr, 10);
            if (*endptr == '\0' || *endptr == '/') {
                Socket::DebugSocket(os, sid);
            } else {
                cntl->SetFailed(ENOMETHOD, "path=%s is not a SocketId",
                                constraint.c_str());
            }
        }
        os.move_to(cntl->response_attachment());
    }

} // namespace melon::rpc
