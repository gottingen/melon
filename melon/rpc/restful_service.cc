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
#include <melon/rpc/restful_service.h>
#include <turbo/strings/str_format.h>
#include <melon/rpc/server.h>

namespace melon {

    void RestfulService::impl_method(::google::protobuf::RpcController *controller,
                                        const ::melon::NoUseRestfulRequest *request,
                                        ::melon::NoUseRestfulResponse *response,
                                        ::google::protobuf::Closure *done) {
        (void) request;
        (void) response;
        melon::ClosureGuard done_guard(done);
        auto *ctrl = dynamic_cast<Controller *>(controller);
        const RestfulRequest req(ctrl);
        RestfulResponse resp(ctrl);
        process(&req, &resp);
    }

    turbo::Status RestfulService::register_server(const std::string &mapping_path, Server *server) {
        if(mapping_path.empty() || server == nullptr) {
            return turbo::invalid_argument_error("mapping_path or server is empty");
        }
        mapping_path_ = mapping_path;
        std::string map_str = turbo::str_format("%s/* => impl_method", mapping_path.c_str());
        auto r = server->AddService(this,
                          melon::SERVER_DOESNT_OWN_SERVICE,
                          map_str);
        if(r != 0) {
            return turbo::internal_error("register restful service failed");
        }
        return turbo::OkStatus();
    }
}  // namespace melon
