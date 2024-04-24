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


#include "melon/builtin/common.h"
#include "melon/builtin/grpc_health_check_service.h"
#include "melon/rpc/controller.h"           // Controller
#include "melon/rpc/closure_guard.h"        // ClosureGuard
#include "melon/rpc/server.h"               // Server

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

