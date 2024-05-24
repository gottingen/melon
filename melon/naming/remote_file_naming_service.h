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

#pragma once

#include "melon/naming/periodic_naming_service.h"
#include "melon/rpc/channel.h"
#include "melon/utility/unique_ptr.h"


namespace melon {
    class Channel;
}  // namespace melon
namespace melon::naming {

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

} // namespace melon::naming
