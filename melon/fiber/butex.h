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


#ifndef MELON_FIBER_BUTEX_H_
#define MELON_FIBER_BUTEX_H_

#include <errno.h>                               // users need to check errno
#include <time.h>                                // timespec
#include <melon/base/macros.h>
#include <melon/fiber/types.h>                       // fiber_t

namespace fiber {

    // Create a butex which is a futex-like 32-bit primitive for synchronizing
    // fibers/pthreads.
    // Returns a pointer to 32-bit data, NULL on failure.
    // NOTE: all butexes are private(not inter-process).
    void *butex_create();

    // Check width of user type before casting.
    template<typename T>
    T *butex_create_checked() {
        static_assert(sizeof(T) == sizeof(int), "sizeof_T_must_equal_int");
        return static_cast<T *>(butex_create());
    }

    // Destroy the butex.
    void butex_destroy(void *butex);

    // Wake up at most 1 thread waiting on |butex|.
    // Returns # of threads woken up.
    int butex_wake(void *butex, bool nosignal = false);

    // Wake up all threads waiting on |butex|.
    // Returns # of threads woken up.
    int butex_wake_all(void *butex, bool nosignal = false);

    // Wake up all threads waiting on |butex| except a fiber whose identifier
    // is |excluded_fiber|. This function does not yield.
    // Returns # of threads woken up.
    int butex_wake_except(void *butex, fiber_t excluded_fiber);

    // Wake up at most 1 thread waiting on |butex1|, let all other threads wait
    // on |butex2| instead.
    // Returns # of threads woken up.
    int butex_requeue(void *butex1, void *butex2);

    // Atomically wait on |butex| if *butex equals |expected_value|, until the
    // butex is woken up by butex_wake*, or CLOCK_REALTIME reached |abstime| if
    // abstime is not NULL.
    // About |abstime|:
    //   Different from FUTEX_WAIT, butex_wait uses absolute time.
    // Returns 0 on success, -1 otherwise and errno is set.
    int butex_wait(void *butex, int expected_value, const timespec *abstime);

}  // namespace fiber

#endif  // MELON_FIBER_BUTEX_H_
