// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#ifndef  MELON_VAR_SCOPED_TIMER_H_
#define  MELON_VAR_SCOPED_TIMER_H_

#include "melon/utility/time.h"

// Accumulate microseconds spent by scopes into bvar, useful for debugging.
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
// can wrap the bvar within PerSecond and make it viewable from /vars
//   melon::var::PerSecond<melon::var::Adder<int64_t> > g_function1_spent_second(
//     "function1_spent_second", &g_function1_spent);
namespace melon::var {
    template<typename T>
    class ScopedTimer {
    public:
        explicit ScopedTimer(T &bvar)
                : _start_time(mutil::cpuwide_time_us()), _bvar(&bvar) {}

        ~ScopedTimer() {
            *_bvar << (mutil::cpuwide_time_us() - _start_time);
        }

        void reset() { _start_time = mutil::cpuwide_time_us(); }

    private:
        DISALLOW_COPY_AND_ASSIGN(ScopedTimer);

        int64_t _start_time;
        T *_bvar;
    };
} // namespace melon::var

#endif  // MELON_VAR_SCOPED_TIMER_H_
