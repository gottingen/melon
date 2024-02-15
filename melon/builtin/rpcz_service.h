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



#ifndef MELON_BUILTIN_RPCZ_SERVICE_H_
#define MELON_BUILTIN_RPCZ_SERVICE_H_

#include "melon/proto/rpc/builtin_service.pb.h"
#include "melon/builtin/tabbed.h"


namespace melon {

    class RpczService : public rpcz, public Tabbed {
    public:
        void enable(::google::protobuf::RpcController *cntl_base,
                    const ::melon::RpczRequest *request,
                    ::melon::RpczResponse *response,
                    ::google::protobuf::Closure *done);

        void disable(::google::protobuf::RpcController *cntl_base,
                     const ::melon::RpczRequest *request,
                     ::melon::RpczResponse *response,
                     ::google::protobuf::Closure *done);

        void stats(::google::protobuf::RpcController *cntl_base,
                   const ::melon::RpczRequest *request,
                   ::melon::RpczResponse *response,
                   ::google::protobuf::Closure *done);

        void hex_log_id(::google::protobuf::RpcController *cntl_base,
                        const ::melon::RpczRequest *request,
                        ::melon::RpczResponse *response,
                        ::google::protobuf::Closure *done);

        void dec_log_id(::google::protobuf::RpcController *cntl_base,
                        const ::melon::RpczRequest *request,
                        ::melon::RpczResponse *response,
                        ::google::protobuf::Closure *done);

        void default_method(::google::protobuf::RpcController *cntl_base,
                            const ::melon::RpczRequest *request,
                            ::melon::RpczResponse *response,
                            ::google::protobuf::Closure *done);

        void GetTabInfo(melon::TabInfoList *) const;
    };

} // namespace melon


#endif // MELON_BUILTIN_RPCZ_SERVICE_H_
