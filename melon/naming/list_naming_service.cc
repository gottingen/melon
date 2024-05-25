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



#include <cstdlib>                                   // strtol
#include <string>                                     // std::string
#include <set>                                        // std::set
#include <melon/utility/string_splitter.h>                     // StringSplitter
#include <melon/rpc/log.h>
#include <melon/naming/list_naming_service.h>


namespace melon::naming {

// Defined in file_naming_service.cpp
    bool SplitIntoServerAndTag(const mutil::StringPiece &line,
                               mutil::StringPiece *server_addr,
                               mutil::StringPiece *tag);

    int ParseServerList(const char *service_name,
                        std::vector<ServerNode> *servers) {
        servers->clear();
        // Sort/unique the inserted vector is faster, but may have a different order
        // of addresses from the file. To make assertions in tests easier, we use
        // set to de-duplicate and keep the order.
        std::set<ServerNode> presence;
        std::string line;

        if (!service_name) {
            MLOG(FATAL) << "Param[service_name] is NULL";
            return -1;
        }
        for (mutil::StringSplitter sp(service_name, ','); sp != NULL; ++sp) {
            line.assign(sp.field(), sp.length());
            mutil::StringPiece addr;
            mutil::StringPiece tag;
            if (!SplitIntoServerAndTag(line, &addr, &tag)) {
                continue;
            }
            const_cast<char *>(addr.data())[addr.size()] = '\0'; // safe
            mutil::EndPoint point;
            if (str2endpoint(addr.data(), &point) != 0 &&
                hostname2endpoint(addr.data(), &point) != 0) {
                MLOG(ERROR) << "Invalid address=`" << addr << '\'';
                continue;
            }
            ServerNode node;
            node.addr = point;
            tag.CopyToString(&node.tag);
            if (presence.insert(node).second) {
                servers->push_back(node);
            } else {
                RPC_VMLOG << "Duplicated server=" << node;
            }
        }
        RPC_VMLOG << "Got " << servers->size()
                 << (servers->size() > 1 ? " servers" : " server");
        return 0;
    }

    int ListNamingService::GetServers(const char *service_name,
                                      std::vector<ServerNode> *servers) {
        return ParseServerList(service_name, servers);
    }

    int ListNamingService::RunNamingService(const char *service_name,
                                            NamingServiceActions *actions) {
        std::vector<ServerNode> servers;
        const int rc = GetServers(service_name, &servers);
        if (rc != 0) {
            servers.clear();
        }
        actions->ResetServers(servers);
        return 0;
    }

    void ListNamingService::Describe(
            std::ostream &os, const DescribeOptions &) const {
        os << "list";
        return;
    }

    NamingService *ListNamingService::New() const {
        return new ListNamingService;
    }

    void ListNamingService::Destroy() {
        delete this;
    }

    int DomainListNamingService::GetServers(const char *service_name,
                                            std::vector<ServerNode> *servers) {
        return ParseServerList(service_name, servers);
    }

    void DomainListNamingService::Describe(std::ostream &os,
                                           const DescribeOptions &) const {
        os << "dlist";
        return;
    }

    NamingService *DomainListNamingService::New() const {
        return new DomainListNamingService;
    }

    void DomainListNamingService::Destroy() { delete this; }

} // namespace melon::naming
