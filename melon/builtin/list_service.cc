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



#include <vector>                           // std::vector
#include <google/protobuf/descriptor.h>     // ServiceDescriptor
#include "melon/rpc/controller.h"           // Controller
#include "melon/rpc/server.h"               // Server
#include "melon/rpc/closure_guard.h"        // ClosureGuard
#include "melon/builtin/list_service.h"


namespace melon {

    void ListService::default_method(::google::protobuf::RpcController *,
                                     const ::melon::ListRequest *,
                                     ::melon::ListResponse *response,
                                     ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        std::vector<google::protobuf::Service *> services;
        _server->ListServices(&services);
        for (size_t i = 0; i < services.size(); ++i) {
            google::protobuf::ServiceDescriptorProto *svc = response->add_service();
            services[i]->GetDescriptor()->CopyTo(svc);
        }
    }

} // namespace melon
