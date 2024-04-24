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


#ifndef MELON_NAMING_NACOS_NAMING_SERVICE_H_
#define MELON_NAMING_NACOS_NAMING_SERVICE_H_

#include <time.h>

#include <string>
#include <vector>

#include "melon/rpc/channel.h"
#include "melon/naming/periodic_naming_service.h"
#include "melon/rpc/server_node.h"

namespace melon::naming {

    // Acquire server list from nacos
    class NacosNamingService : public PeriodicNamingService {
    public:
        NacosNamingService();

        int GetServers(const char *service_name,
                       std::vector<ServerNode> *servers) override;

        int GetNamingServiceAccessIntervalMs() const override;

        void Describe(std::ostream &os, const DescribeOptions &) const override;

        NamingService *New() const override;

        void Destroy() override;

    private:
        int Connect();

        int RefreshAccessToken(const char *service_name);

        int GetServerNodes(const char *service_name, bool token_changed,
                           std::vector<ServerNode> *nodes);

    private:
        melon::Channel _channel;
        std::string _nacos_url;
        std::string _access_token;
        bool _nacos_connected;
        long _cache_ms;
        time_t _token_expire_time;
    };

}  // namespace melon::naming

#endif  // MELON_NAMING_NACOS_NAMING_SERVICE_H_
