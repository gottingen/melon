// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//



#include "melon/rpc/controller.h"           // Controller
#include "melon/rpc/server.h"               // Server
#include "melon/rpc/closure_guard.h"        // ClosureGuard
#include "melon/builtin/version_service.h"


namespace melon {

    void VersionService::default_method(::google::protobuf::RpcController *controller,
                                        const ::melon::VersionRequest *,
                                        ::melon::VersionResponse *,
                                        ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = (Controller *) controller;
        cntl->http_response().set_content_type("text/plain");
        if (_server->version().empty()) {
            cntl->response_attachment().append("unknown");
        } else {
            cntl->response_attachment().append(_server->version());
        }
    }

} // namespace melon
