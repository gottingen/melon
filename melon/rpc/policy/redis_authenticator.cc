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


#include "melon/rpc/policy/redis_authenticator.h"

#include "melon/utility/base64.h"
#include "melon/utility/iobuf.h"
#include "melon/utility/string_printf.h"
#include "melon/utility/sys_byteorder.h"
#include "melon/rpc/redis/redis_command.h"

namespace melon {
namespace policy {

int RedisAuthenticator::GenerateCredential(std::string* auth_str) const {
    mutil::IOBuf buf;
    if (!passwd_.empty()) {
        melon::RedisCommandFormat(&buf, "AUTH %s", passwd_.c_str());
    }
    if (db_ >= 0) {
        melon::RedisCommandFormat(&buf, "SELECT %d", db_);
    }
    *auth_str = buf.to_string();
    return 0;
}

}  // namespace policy
}  // namespace melon
