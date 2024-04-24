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

#pragma once

#include <vector>                                      // std::vector
#include <map>                                         // std::map
#include "melon/utility/containers/doubly_buffered_data.h"
#include "melon/rpc/load_balancer.h"


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
    DynPartLoadBalancer* New(const mutil::StringPiece&) const;
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

