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



#ifndef MELON_RPC_CHANNEL_BASE_H_
#define MELON_RPC_CHANNEL_BASE_H_

#include <stdlib.h>
#include <ostream>
#include "melon/utility/logging.h"
#include <google/protobuf/service.h>            // google::protobuf::RpcChannel
#include "melon/rpc/describable.h"




namespace melon {

// Base of all melon channels.
class ChannelBase : public google::protobuf::RpcChannel/*non-copyable*/,
                    public Describable {
public:
    virtual int Weight() {
        MCHECK(false) << "Not implemented";
        abort();
    };

    virtual int CheckHealth() = 0;
};

} // namespace melon


#endif  // MELON_RPC_CHANNEL_BASE_H_
