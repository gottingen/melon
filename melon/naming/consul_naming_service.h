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



#ifndef  MELON_NAMING_CONSUL_NAMING_SERVICE_H_
#define  MELON_NAMING_CONSUL_NAMING_SERVICE_H_

#include <melon/naming/naming_service.h>
#include <melon/rpc/channel.h>


namespace melon {
    class Channel;
}  // namespace melon
namespace melon::naming {

    class ConsulNamingService : public NamingService {
    private:
        int RunNamingService(const char *service_name,
                             NamingServiceActions *actions) override;

        int GetServers(const char *service_name,
                       std::vector<ServerNode> *servers);

        void Describe(std::ostream &os, const DescribeOptions &) const override;

        NamingService *New() const override;

        int DegradeToOtherServiceIfNeeded(const char *service_name,
                                          std::vector<ServerNode> *servers);

        void Destroy() override;

    private:
        Channel _channel;
        std::string _consul_index;
        std::string _consul_url;
        bool _backup_file_loaded = false;
        bool _consul_connected = false;
    };

} // namespace melon::naming


#endif  // MELON_NAMING_CONSUL_NAMING_SERVICE_H_