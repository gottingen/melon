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

#include "melon/rpc/policy/redis_authenticator.h"

#include "melon/base/base64.h"
#include "melon/io/cord_buf.h"
#include "melon/strings/str_format.h"
#include "melon/base/endian.h"
#include "melon/rpc/redis_command.h"

namespace melon::rpc {
    namespace policy {

        int RedisAuthenticator::GenerateCredential(std::string *auth_str) const {
            melon::cord_buf buf;
            melon::rpc::RedisCommandFormat(&buf, "AUTH %s", passwd_.c_str());
            *auth_str = buf.to_string();
            return 0;
        }

    }  // namespace policy
}  // namespace melon::rpc
