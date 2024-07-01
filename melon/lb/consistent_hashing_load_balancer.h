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

#include <stdint.h>                                     // uint32_t
#include <functional>
#include <vector>                                       // std::vector
#include <melon/utility/endpoint.h>                              // mutil::EndPoint
#include <melon/utility/containers/doubly_buffered_data.h>
#include <melon/rpc/load_balancer.h>


namespace melon::lb {

    class ReplicaPolicy;

    enum ConsistentHashingLoadBalancerType {
        CONS_HASH_LB_MURMUR3 = 0,
        CONS_HASH_LB_MD5 = 1,
        CONS_HASH_LB_KETAMA = 2,

        // Identify the last one.
        CONS_HASH_LB_LAST = 3
    };

    class ConsistentHashingLoadBalancer : public LoadBalancer {
    public:
        struct Node {
            uint32_t hash;
            ServerId server_sock;
            mutil::EndPoint server_addr;  // To make sorting stable among all clients
            bool operator<(const Node &rhs) const {
                if (hash < rhs.hash) { return true; }
                if (hash > rhs.hash) { return false; }
                return server_addr < rhs.server_addr;
            }

            bool operator<(const uint32_t code) const {
                return hash < code;
            }
        };

        explicit ConsistentHashingLoadBalancer(ConsistentHashingLoadBalancerType type);

        bool AddServer(const ServerId &server);

        bool RemoveServer(const ServerId &server);

        size_t AddServersInBatch(const std::vector<ServerId> &servers);

        size_t RemoveServersInBatch(const std::vector<ServerId> &servers);

        LoadBalancer *New(const std::string_view &params) const;

        void Destroy();

        int SelectServer(const SelectIn &in, SelectOut *out);

        void Describe(std::ostream &os, const DescribeOptions &options);

    private:
        bool SetParameters(const std::string_view &params);

        void GetLoads(std::map<mutil::EndPoint, double> *load_map);

        static size_t AddBatch(std::vector<Node> &bg, const std::vector<Node> &fg,
                               const std::vector<Node> &servers, bool *executed);

        static size_t RemoveBatch(std::vector<Node> &bg, const std::vector<Node> &fg,
                                  const std::vector<ServerId> &servers, bool *executed);

        static size_t Remove(std::vector<Node> &bg, const std::vector<Node> &fg,
                             const ServerId &server, bool *executed);

        size_t _num_replicas;
        ConsistentHashingLoadBalancerType _type;
        mutil::DoublyBufferedData<std::vector<Node> > _db_hash_ring;
    };

} // namespace melon::lb
