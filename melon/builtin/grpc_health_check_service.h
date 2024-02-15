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



#ifndef MELON_BUILTIN_GRPC_HEALTH_CHECK_SERVICE_H_
#define MELON_BUILTIN_GRPC_HEALTH_CHECK_SERVICE_H_

#include "melon/proto/rpc/grpc_health_check.pb.h"

namespace melon {

class GrpcHealthCheckService : public grpc::health::v1::Health {
public:
    void Check(::google::protobuf::RpcController* cntl_base,
               const grpc::health::v1::HealthCheckRequest* request,
               grpc::health::v1::HealthCheckResponse* response,
               ::google::protobuf::Closure* done);

};

} // namespace melon

#endif // MELON_BUILTIN_GRPC_HEALTH_CHECK_SERVICE_H_
