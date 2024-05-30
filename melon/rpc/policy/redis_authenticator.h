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

#include <melon/rpc/authenticator.h>

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
