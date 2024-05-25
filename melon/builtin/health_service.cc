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
#include <melon/builtin/health_service.h>
#include <melon/builtin/common.h>


namespace melon {

    void HealthService::default_method(::google::protobuf::RpcController *cntl_base,
                                       const ::melon::HealthRequest *,
                                       ::melon::HealthResponse *,
                                       ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        const Server *server = cntl->server();
        if (server->options().health_reporter) {
            server->options().health_reporter->GenerateReport(
                    cntl, done_guard.release());
        } else {
            cntl->http_response().set_content_type("text/plain");
            cntl->response_attachment().append("OK");
        }
    }

} // namespace melon
