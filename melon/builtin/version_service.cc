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



#include <melon/rpc/controller.h>           // Controller
#include <melon/rpc/server.h>               // Server
#include <melon/rpc/closure_guard.h>        // ClosureGuard
#include <melon/builtin/version_service.h>


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
