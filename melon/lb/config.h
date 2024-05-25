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

namespace melon::lb {

    // default number of replicas per server in chash default 100
    DECLARE_int32(chash_num_replicas);

    // Minimum weight of a node in LALB default 1000
    DECLARE_int64(min_weight);

    // Decrease weight proportionally if average latency of
    // the inflight requests exeeds average latency of the
    // node times this ratio default 1.5
    DECLARE_double(punish_inflight_ratio);

    // Multiply latencies caused by errors with this ratio default 1.2
    DECLARE_double(punish_error_ratio);
}  // namespace melon::var
