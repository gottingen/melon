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

#include <turbo/flags/declare.h>
TURBO_DECLARE_FLAG(std::string, rpc_profiling_dir);
TURBO_DECLARE_FLAG(bool, show_hostname_instead_of_ip);

// Print stats of at most so many connections default 1024
TURBO_DECLARE_FLAG(int32_t, max_shown_connections);

// gflags on /flags page can't be modified default false
TURBO_DECLARE_FLAG(bool, immutable_flags);

// max length of TCMalloc stats default 32 * 1024
TURBO_DECLARE_FLAG(int32_t ,max_tc_stats_buf_len);

// turn on/off rpcz default false
TURBO_DECLARE_FLAG(bool, enable_rpcz);

// show log_id in hexadecimal default false
TURBO_DECLARE_FLAG(bool, rpcz_hex_log_id);

TURBO_DECLARE_FLAG(bool ,enable_dir_service);
TURBO_DECLARE_FLAG(bool ,enable_threads_service);
TURBO_DECLARE_FLAG(bool ,quote_vector);

