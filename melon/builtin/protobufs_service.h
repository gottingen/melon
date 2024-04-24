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



#ifndef  MELON_BUILTIN_PROTOBUFS_SERVICE_H_
#define  MELON_BUILTIN_PROTOBUFS_SERVICE_H_

#include <ostream>
#include "melon/proto/rpc/builtin_service.pb.h"


namespace melon {

    class Server;

    // Show DebugString of protobuf messages used in the server.
    //   /protobufs         : list all supported messages.
    //   /protobufs/<msg>/  : Show DebugString() of <msg>

    class ProtobufsService : public protobufs {
    public:
        explicit ProtobufsService(Server *server);

        void default_method(::google::protobuf::RpcController *cntl_base,
                            const ::melon::ProtobufsRequest *request,
                            ::melon::ProtobufsResponse *response,
                            ::google::protobuf::Closure *done);

    private:
        int Init();

        Server *_server;
        typedef std::map<std::string, std::string> Map;
        Map _map;
    };

} // namespace melon


#endif  // MELON_BUILTIN_PROTOBUFS_SERVICE_H_
