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

#include <melon/utility/time.h>

// Accumulate microseconds spent by scopes into var, useful for debugging.
// Example:
//   melon::var::Adder<int64_t> g_function1_spent;
//   ...
//   void function1() {
//     // time cost by function1() will be sent to g_spent_time when
//     // the function returns.
//     melon::var::ScopedTimer tm(g_function1_spent);
//     ...
//   }
// To check how many microseconds the function spend in last second, you
// can wrap the var within PerSecond and make it viewable from /vars
//   melon::var::PerSecond<melon::var::Adder<int64_t> > g_function1_spent_second(
//     "function1_spent_second", &g_function1_spent);
namespace melon::var {
    template<typename T>
    class ScopedTimer {
    public:
        explicit ScopedTimer(T &var)
                : _start_time(mutil::cpuwide_time_us()), _var(&var) {}

        ~ScopedTimer() {
            *_var << (mutil::cpuwide_time_us() - _start_time);
        }

        void reset() { _start_time = mutil::cpuwide_time_us(); }

    private:
        DISALLOW_COPY_AND_ASSIGN(ScopedTimer);

        int64_t _start_time;
        T *_var;
    };
} // namespace melon::var
