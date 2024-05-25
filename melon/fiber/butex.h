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


#ifndef MELON_FIBER_BUTEX_H_
#define MELON_FIBER_BUTEX_H_

#include <errno.h>                               // users need to check errno
#include <time.h>                                // timespec
#include "melon/utility/macros.h"                         // MELON_CASSERT
#include "melon/fiber/types.h"                       // fiber_t

namespace fiber {

    // Create a butex which is a futex-like 32-bit primitive for synchronizing
    // fibers/pthreads.
    // Returns a pointer to 32-bit data, NULL on failure.
    // NOTE: all butexes are private(not inter-process).
    void *butex_create();

    // Check width of user type before casting.
    template<typename T>
    T *butex_create_checked() {
        MELON_CASSERT(sizeof(T) == sizeof(int), sizeof_T_must_equal_int);
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
