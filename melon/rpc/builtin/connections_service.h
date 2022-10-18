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


#ifndef MELON_RPC_CONNECTIONS_SERVICE_H_
#define MELON_RPC_CONNECTIONS_SERVICE_H_

#include "melon/rpc/socket_id.h"
#include "melon/rpc/builtin_service.pb.h"
#include "melon/rpc/builtin/tabbed.h"


namespace melon::rpc {

    class Acceptor;

    class ConnectionsService : public connections, public Tabbed {
    public:
        void default_method(::google::protobuf::RpcController *cntl_base,
                            const ::melon::rpc::ConnectionsRequest *request,
                            ::melon::rpc::ConnectionsResponse *response,
                            ::google::protobuf::Closure *done);

        void GetTabInfo(TabInfoList *info_list) const;

    private:
        void PrintConnections(std::ostream &os, const std::vector<SocketId> &conns,
                              bool use_html, const Server *, bool need_local) const;
    };

} // namespace melon::rpc


#endif // MELON_RPC_CONNECTIONS_SERVICE_H_
