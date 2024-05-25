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


#include <melon/rpc/policy/couchbase_authenticator.h>

#include <melon/utility/base64.h>
#include <melon/utility/iobuf.h>
#include <melon/utility/string_printf.h>
#include <melon/utility/sys_byteorder.h>
#include <melon/rpc/policy/memcache_binary_header.h>

namespace melon {
    namespace policy {

        namespace {

            constexpr char kPlainAuthCommand[] = "PLAIN";
            constexpr char kPadding[1] = {'\0'};

        }  // namespace

        // To get the couchbase authentication protocol, see
        // https://developer.couchbase.com/documentation/server/3.x/developer/dev-guide-3.0/sasl.html
        int CouchbaseAuthenticator::GenerateCredential(std::string *auth_str) const {
            const melon::policy::MemcacheRequestHeader header = {
                    melon::policy::MC_MAGIC_REQUEST, melon::policy::MC_BINARY_SASL_AUTH,
                    mutil::HostToNet16(sizeof(kPlainAuthCommand) - 1), 0, 0, 0,
                    mutil::HostToNet32(sizeof(kPlainAuthCommand) + 1 +
                                       bucket_name_.length() * 2 + bucket_password_.length()),
                    0, 0};
            auth_str->clear();
            auth_str->append(reinterpret_cast<const char *>(&header), sizeof(header));
            auth_str->append(kPlainAuthCommand, sizeof(kPlainAuthCommand) - 1);
            auth_str->append(bucket_name_);
            auth_str->append(kPadding, sizeof(kPadding));
            auth_str->append(bucket_name_);
            auth_str->append(kPadding, sizeof(kPadding));
            auth_str->append(bucket_password_);
            return 0;
        }

    }  // namespace policy
}  // namespace melon
