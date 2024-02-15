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


#ifndef  MELON_LB_CONSISTENT_HASHING_LOAD_BALANCER_H_
#define  MELON_LB_CONSISTENT_HASHING_LOAD_BALANCER_H_

#include <stdint.h>                                     // uint32_t
#include <functional>
#include <vector>                                       // std::vector
#include "melon/utility/endpoint.h"                              // mutil::EndPoint
#include "melon/utility/containers/doubly_buffered_data.h"
#include "melon/rpc/load_balancer.h"


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

        LoadBalancer *New(const mutil::StringPiece &params) const;

        void Destroy();

        int SelectServer(const SelectIn &in, SelectOut *out);

        void Describe(std::ostream &os, const DescribeOptions &options);

    private:
        bool SetParameters(const mutil::StringPiece &params);

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


#endif  // MELON_LB_CONSISTENT_HASHING_LOAD_BALANCER_H_
