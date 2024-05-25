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



#include <melon/rpc/ssl_options.h>

namespace melon {

VerifyOptions::VerifyOptions() : verify_depth(0) {}

ChannelSSLOptions::ChannelSSLOptions()
    : ciphers("DEFAULT")
    , protocols("TLSv1, TLSv1.1, TLSv1.2")
{}

ServerSSLOptions::ServerSSLOptions()
    : strict_sni(false)
    , disable_ssl3(true)
    , release_buffer(false)
    , session_lifetime_s(300)
    , session_cache_size(20480)
    , ecdhe_curve_name("prime256v1")
{}

} // namespace melon
