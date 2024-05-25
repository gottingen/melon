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


#include <melon/builtin/common.h>
#include <melon/builtin/grpc_health_check_service.h>
#include <melon/rpc/controller.h>           // Controller
#include <melon/rpc/closure_guard.h>        // ClosureGuard
#include <melon/rpc/server.h>               // Server

namespace melon {
    void GrpcHealthCheckService::Check(::google::protobuf::RpcController *cntl_base,
                                       const grpc::health::v1::HealthCheckRequest *request,
                                       grpc::health::v1::HealthCheckResponse *response,
                                       ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        const Server *server = cntl->server();
        if (server->options().health_reporter) {
            server->options().health_reporter->GenerateReport(
                    cntl, done_guard.release());
        } else {
            response->set_status(grpc::health::v1::HealthCheckResponse_ServingStatus_SERVING);
        }
    }

} // namespace melon

