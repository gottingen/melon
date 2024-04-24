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

#include "melon/rpc/authenticator.h"

namespace melon {
namespace policy {

// Request to redis for authentication.
class RedisAuthenticator : public Authenticator {
public:
    RedisAuthenticator(const std::string& passwd, int db = -1)
        : passwd_(passwd), db_(db) {}

    int GenerateCredential(std::string* auth_str) const;

    int VerifyCredential(const std::string&, const mutil::EndPoint&,
                         melon::AuthContext*) const {
        return 0;
    }

    uint32_t GetAuthFlags() const {
        uint32_t n = 0;
        if (!passwd_.empty()) {
            ++n;
        }
        if (db_ >= 0) {
            ++n;
        }
        return n;
    }

private:
    const std::string passwd_;

    int db_;
};

}  // namespace policy
}  // namespace melon
