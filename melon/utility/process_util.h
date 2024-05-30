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


// Process-related Info

// Date: Wed Apr 11 14:35:56 CST 2018

#ifndef MUTIL_PROCESS_UTIL_H
#define MUTIL_PROCESS_UTIL_H

#include <sys/types.h>

namespace mutil {

// Read command line of this program. If `with_args' is true, args are
// included and separated with spaces.
// Returns length of the command line on sucess, -1 otherwise.
// NOTE: `buf' does not end with zero.
ssize_t ReadCommandLine(char* buf, size_t len, bool with_args);

} // namespace mutil

#endif // MUTIL_PROCESS_UTIL_H
