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

#include <gflags/gflags_declare.h>

namespace melon {

    /// builtin flags
    // For storing profiling results
    DECLARE_string(rpc_profiling_dir);

    // shows hostname instead of ip default false
    DECLARE_bool(show_hostname_instead_of_ip);

    // Print stats of at most so many connections default 1024
    DECLARE_int32(max_shown_connections);

    // gflags on /flags page can't be modified default false
    DECLARE_bool(immutable_flags);

    // max length of TCMalloc stats default 32 * 1024
    DECLARE_int32(max_tc_stats_buf_len);

    // turn on/off rpcz default false
    DECLARE_bool(enable_rpcz);

    // show log_id in hexadecimal default false
    DECLARE_bool(rpcz_hex_log_id);

    DECLARE_bool(enable_dir_service);
    DECLARE_bool(enable_threads_service);
    DECLARE_bool(quote_vector);
}  // namespace melon
