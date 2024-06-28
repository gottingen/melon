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

#include <melon/utility/atomicops.h>
#include <melon/fiber/fiber.h>
#include <turbo/flags/declare.h>
#include <turbo/flags/flag.h>
TURBO_DECLARE_FLAG(bool, usercode_in_pthread);
TURBO_DECLARE_FLAG(int, usercode_backup_threads);

namespace melon {

// "user code backup pool" is a set of pthreads to run user code when #pthread
// workers of fibers reaches a threshold, avoiding potential deadlock when
// -usercode_in_pthread is on. These threads are NOT supposed to be active
// frequently, if they're, user should configure more num_threads for the
// server or set -fiber_concurrency to a larger value.

// Run the user code in-place or in backup threads. The place depends on
// busy-ness of fiber workers.
// NOTE: To avoid memory allocation(for `arg') in the case of in-place running,
// check out the inline impl. of this function just below.
void RunUserCode(void (*fn)(void*), void* arg);

// RPC code should check this function before submitting operations that have
// user code to run laterly.
inline bool TooManyUserCode() {
    extern bool g_too_many_usercode;
    return g_too_many_usercode;
}

// If this function returns true, the user code is suggested to be run in-place
// and user should call EndRunningUserCodeInPlace() after running the code.
// Otherwise, user should call EndRunningUserCodeInPool() to run the user code
// in backup threads.
// Check RunUserCode() below to see the usage pattern.
inline bool BeginRunningUserCode() {
    extern mutil::static_atomic<int> g_usercode_inplace;
    return (g_usercode_inplace.fetch_add(1, mutil::memory_order_relaxed)
            + turbo::get_flag(FLAGS_usercode_backup_threads) < fiber_getconcurrency());
}

inline void EndRunningUserCodeInPlace() {
    extern mutil::static_atomic<int> g_usercode_inplace;
    g_usercode_inplace.fetch_sub(1, mutil::memory_order_relaxed);
}

void EndRunningUserCodeInPool(void (*fn)(void*), void* arg);

// Incorporate functions above together. However `arg' to this function often
// has to be new-ed even for in-place cases. If performance is critical, use
// the BeginXXX/EndXXX pattern.
inline void RunUserCode(void (*fn)(void*), void* arg) {
    if (BeginRunningUserCode()) {
        fn(arg);
        EndRunningUserCodeInPlace();
    } else {
        EndRunningUserCodeInPool(fn, arg);
    }
}

// [Optional] initialize the pool of backup threads. If this function is not
// called, it will be called in EndRunningUserCodeInPool
void InitUserCodeBackupPoolOnceOrDie();

} // namespace melon
