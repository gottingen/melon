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


#ifndef MELON_LB_POLICY_WEIGHTED_ROUND_ROBIN_LOAD_BALANCER_H_
#define MELON_LB_POLICY_WEIGHTED_ROUND_ROBIN_LOAD_BALANCER_H_

#include <map>
#include <vector>
#include <unordered_set>
#include <melon/utility/containers/doubly_buffered_data.h>
#include <melon/rpc/load_balancer.h>

namespace melon::lb {

// This LoadBalancer selects server as the assigned weight.
// Weight is got from tag of ServerId.
    class WeightedRoundRobinLoadBalancer : public LoadBalancer {
    public:
        bool AddServer(const ServerId &id);

        bool RemoveServer(const ServerId &id);

        size_t AddServersInBatch(const std::vector<ServerId> &servers);

        size_t RemoveServersInBatch(const std::vector<ServerId> &servers);

        int SelectServer(const SelectIn &in, SelectOut *out);

        LoadBalancer *New(const mutil::StringPiece &) const;

        void Destroy();

        void Describe(std::ostream &, const DescribeOptions &options);

    private:
        struct Server {
            Server(SocketId s_id = 0, uint32_t s_w = 0) : id(s_id), weight(s_w) {}

            SocketId id;
            uint32_t weight;
        };

        struct Servers {
            // The value is configured weight for each server.
            std::vector<Server> server_list;
            // The value is the index of the server in "server_list".
            std::map<SocketId, size_t> server_map;
            uint64_t weight_sum = 0;
        };

        struct TLS {
            size_t position = 0;
            uint64_t stride = 0;
            Server remain_server;

            // If server list changed, we need calculate a new stride.
            bool IsNeededCalculateNewStride(const uint64_t curr_weight_sum,
                                            const size_t curr_servers_num) {
                if (curr_weight_sum != weight_sum
                    || curr_servers_num != servers_num) {
                    weight_sum = curr_weight_sum;
                    servers_num = curr_servers_num;
                    return true;
                }
                return false;
            }

        private:
            uint64_t weight_sum = 0;
            size_t servers_num = 0;
        };

        static bool Add(Servers &bg, const ServerId &id);

        static bool Remove(Servers &bg, const ServerId &id);

        static size_t BatchAdd(Servers &bg, const std::vector<ServerId> &servers);

        static size_t BatchRemove(Servers &bg, const std::vector<ServerId> &servers);

        static SocketId GetServerInNextStride(const std::vector<Server> &server_list,
                                              const std::unordered_set<SocketId> &filter,
                                              TLS &tls);

        mutil::DoublyBufferedData<Servers, TLS> _db_servers;
    };

} // namespace melon::lb

#endif  // MELON_LB_POLICY_WEIGHTED_ROUND_ROBIN_LOAD_BALANCER_H_
