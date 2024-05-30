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

namespace melon::var {

    DECLARE_int32(var_latency_p1);
    DECLARE_int32(var_latency_p2);
    DECLARE_int32(var_latency_p3);
    DECLARE_int32(var_max_dump_multi_dimension_metric_number);
    DECLARE_bool(quote_vector);
}  // namespace melon::var
