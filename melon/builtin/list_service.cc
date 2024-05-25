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



#include <vector>                           // std::vector
#include <google/protobuf/descriptor.h>     // ServiceDescriptor
#include <melon/rpc/controller.h>           // Controller
#include <melon/rpc/server.h>               // Server
#include <melon/rpc/closure_guard.h>        // ClosureGuard
#include <melon/builtin/list_service.h>


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
