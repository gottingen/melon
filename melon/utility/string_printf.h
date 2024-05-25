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


#ifndef MUTIL_STRING_PRINTF_H
#define MUTIL_STRING_PRINTF_H

#include <string>                                // std::string
#include <stdarg.h>                              // va_list

namespace mutil {

// Convert |format| and associated arguments to std::string
std::string string_printf(const char* format, ...)
    __attribute__ ((format (printf, 1, 2)));

// Hint size of formatted string.
std::string string_printf(size_t hint_size, const char* format, ...)
    __attribute__ ((format (printf, 2, 3)));

// Write |format| and associated arguments into |output|
// Returns 0 on success, -1 otherwise.
int string_printf(std::string* output, const char* format, ...)
    __attribute__ ((format (printf, 2, 3)));

// Write |format| and associated arguments in form of va_list into |output|.
// Returns 0 on success, -1 otherwise.
int string_vprintf(std::string* output, const char* format, va_list args);

// Append |format| and associated arguments to |output|
// Returns 0 on success, -1 otherwise.
int string_appendf(std::string* output, const char* format, ...)
    __attribute__ ((format (printf, 2, 3)));

// Append |format| and associated arguments in form of va_list to |output|.
// Returns 0 on success, -1 otherwise.
int string_vappendf(std::string* output, const char* format, va_list args);

}  // namespace mutil

#endif  // MUTIL_STRING_PRINTF_H
