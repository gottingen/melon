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


#include <melon/rpc/policy/redis_authenticator.h>
#include <melon/base/iobuf.h>
#include <melon/rpc/redis/redis_command.h>

namespace melon::policy {

    int RedisAuthenticator::GenerateCredential(std::string *auth_str) const {
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

}  // namespace melon::policy
