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


#ifndef  MELON_RPC_POLICY_REMOTE_FILE_NAMING_SERVICE_H_
#define  MELON_RPC_POLICY_REMOTE_FILE_NAMING_SERVICE_H_

#include "melon/rpc/periodic_naming_service.h"
#include "melon/rpc/channel.h"
#include <memory>


namespace melon::rpc {
    class Channel;
    namespace policy {

        class RemoteFileNamingService : public PeriodicNamingService {
        private:
            int GetServers(const char *service_name,
                           std::vector<ServerNode> *servers) override;

            void Describe(std::ostream &os, const DescribeOptions &) const override;

            NamingService *New() const override;

            void Destroy() override;

        private:
            std::unique_ptr<Channel> _channel;
            std::string _server_addr;
            std::string _path;
        };

    }  // namespace policy
} // namespace melon::rpc


#endif  //MELON_RPC_POLICY_REMOTE_FILE_NAMING_SERVICE_H_
