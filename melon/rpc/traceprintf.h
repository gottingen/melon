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



#ifndef MELON_RPC_TRACEPRINTF_H_
#define MELON_RPC_TRACEPRINTF_H_

#include <melon/base/macros.h>
#include <melon/rpc/span.h>

/*

namespace melon {

bool CanAnnotateSpan();
template<typename... Args>
void AnnotateSpan(const turbo::FormatSpec<Args...> &fmt, Args... args);
} // namespace melon
*/

// Use this macro to print log to /rpcz and tracing system.
// If rpcz is not enabled, arguments to this macro is NOT evaluated, don't
// have (critical) side effects in arguments.
#define TRACEPRINTF(fmt, args...)                                       \
    do {                                                                \
        if (::melon::CanAnnotateSpan()) {                          \
            ::melon::AnnotateSpan("[" __FILE__ ":" MELON_SYMBOLSTR(__LINE__) "] " fmt, ##args);           \
        }                                                               \
    } while (0)

#endif  // MELON_RPC_TRACEPRINTF_H_
