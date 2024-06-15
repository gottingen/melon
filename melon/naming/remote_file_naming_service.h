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

#include <melon/naming/periodic_naming_service.h>
#include <melon/rpc/channel.h>
#include <memory>


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
