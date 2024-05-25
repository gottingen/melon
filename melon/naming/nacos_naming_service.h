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

#pragma once

#include <ctime>
#include <string>
#include <vector>

#include <melon/rpc/channel.h>
#include <melon/naming/periodic_naming_service.h>
#include <melon/rpc/server_node.h>

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
