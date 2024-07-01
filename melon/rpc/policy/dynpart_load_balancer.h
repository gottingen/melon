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

#include <vector>                                      // std::vector
#include <map>                                         // std::map
#include <melon/utility/containers/doubly_buffered_data.h>
#include <melon/rpc/load_balancer.h>


namespace melon {
namespace policy {

// CAUTION: This is just a quick/hacking impl. for loading balancing between
// partchans in a DynamicPartitionChannel. Any details are subject to change.

class DynPartLoadBalancer : public LoadBalancer {
public:
    bool AddServer(const ServerId& id);
    bool RemoveServer(const ServerId& id);
    size_t AddServersInBatch(const std::vector<ServerId>& servers);
    size_t RemoveServersInBatch(const std::vector<ServerId>& servers);
    int SelectServer(const SelectIn& in, SelectOut* out);
    DynPartLoadBalancer* New(const std::string_view&) const;
    void Destroy();
    void Describe(std::ostream&, const DescribeOptions& options);

private:
    struct Servers {
        std::vector<ServerId> server_list;
        std::map<ServerId, size_t> server_map;
    };
    static bool Add(Servers& bg, const ServerId& id);
    static bool Remove(Servers& bg, const ServerId& id);
    static size_t BatchAdd(Servers& bg, const std::vector<ServerId>& servers);
    static size_t BatchRemove(Servers& bg, const std::vector<ServerId>& servers);

    mutil::DoublyBufferedData<Servers> _db_servers;
};

}  // namespace policy
} // namespace melon

