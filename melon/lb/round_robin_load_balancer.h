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


#ifndef MELON_LB_POLICY_ROUND_ROBIN_LOAD_BALANCER_H_
#define MELON_LB_POLICY_ROUND_ROBIN_LOAD_BALANCER_H_

#include <vector>                                      // std::vector
#include <map>                                         // std::map
#include <melon/utility/containers/doubly_buffered_data.h>
#include <melon/rpc/load_balancer.h>
#include <melon/rpc/cluster_recover_policy.h>

namespace melon::lb {

    // This LoadBalancer selects server evenly. Selected numbers of servers(added
    // at the same time) are very close.
    class RoundRobinLoadBalancer : public LoadBalancer {
    public:
        bool AddServer(const ServerId &id);

        bool RemoveServer(const ServerId &id);

        size_t AddServersInBatch(const std::vector<ServerId> &servers);

        size_t RemoveServersInBatch(const std::vector<ServerId> &servers);

        int SelectServer(const SelectIn &in, SelectOut *out);

        RoundRobinLoadBalancer *New(const std::string_view &) const;

        void Destroy();

        void Describe(std::ostream &, const DescribeOptions &options);

    private:
        struct Servers {
            std::vector<ServerId> server_list;
            std::map<ServerId, size_t> server_map;
        };

        struct TLS {
            TLS() : stride(0), offset(0) {}

            uint32_t stride;
            uint32_t offset;
        };

        bool SetParameters(const std::string_view &params);

        static bool Add(Servers &bg, const ServerId &id);

        static bool Remove(Servers &bg, const ServerId &id);

        static size_t BatchAdd(Servers &bg, const std::vector<ServerId> &servers);

        static size_t BatchRemove(Servers &bg, const std::vector<ServerId> &servers);

        mutil::DoublyBufferedData<Servers, TLS> _db_servers;
        std::shared_ptr<ClusterRecoverPolicy> _cluster_recover_policy;
    };

} // namespace melon::lb


#endif  // MELON_LB_POLICY_ROUND_ROBIN_LOAD_BALANCER_H_
