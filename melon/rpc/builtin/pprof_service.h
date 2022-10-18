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

#ifndef  MELON_RPC_PPROF_SERVICE_H_
#define  MELON_RPC_PPROF_SERVICE_H_

#include "melon/rpc/builtin_service.pb.h"


namespace melon::rpc {

    class PProfService : public pprof {
    public:
        void profile(::google::protobuf::RpcController *controller,
                     const ::melon::rpc::ProfileRequest *request,
                     ::melon::rpc::ProfileResponse *response,
                     ::google::protobuf::Closure *done);

        void contention(::google::protobuf::RpcController *controller,
                        const ::melon::rpc::ProfileRequest *request,
                        ::melon::rpc::ProfileResponse *response,
                        ::google::protobuf::Closure *done);

        void heap(::google::protobuf::RpcController *controller,
                  const ::melon::rpc::ProfileRequest *request,
                  ::melon::rpc::ProfileResponse *response,
                  ::google::protobuf::Closure *done);

        void growth(::google::protobuf::RpcController *controller,
                    const ::melon::rpc::ProfileRequest *request,
                    ::melon::rpc::ProfileResponse *response,
                    ::google::protobuf::Closure *done);

        void symbol(::google::protobuf::RpcController *controller,
                    const ::melon::rpc::ProfileRequest *request,
                    ::melon::rpc::ProfileResponse *response,
                    ::google::protobuf::Closure *done);

        void cmdline(::google::protobuf::RpcController *controller,
                     const ::melon::rpc::ProfileRequest *request,
                     ::melon::rpc::ProfileResponse *response,
                     ::google::protobuf::Closure *done);
    };

} // namespace melon::rpc


#endif  // MELON_RPC_PPROF_SERVICE_H_
