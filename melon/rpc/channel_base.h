// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.


#ifndef MELON_RPC_CHANNEL_BASE_H_
#define MELON_RPC_CHANNEL_BASE_H_

#include <cstdlib>
#include <ostream>
#include "melon/log/logging.h"
#include <google/protobuf/service.h>            // google::protobuf::RpcChannel
#include "melon/rpc/describable.h"

// To melon developers: This is a header included by user, don't depend
// on internal structures, use opaque pointers instead.


namespace melon::rpc {

    // Base of all melon/rpc channels.
    class ChannelBase : public google::protobuf::RpcChannel/*non-copyable*/,
                        public Describable {
    public:
        virtual int Weight() {
            MELON_CHECK(false) << "Not implemented";
            abort();
        };

        virtual int CheckHealth() = 0;
    };

} // namespace melon::rpc


#endif  // MELON_RPC_CHANNEL_BASE_H_
