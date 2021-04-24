// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/fiber/internal/spin_lock.h"


#if defined(__x86_64__)
#define ABEL_CPU_RELAX() asm volatile("pause" ::: "memory")
#else
// TODO(bohuli): Implement this macro for non-x86 ISAs.
#define ABEL_CPU_RELAX()
#endif


// Description of lock-word:
//  31..00: [............................3][2][1][0]
//
//     [0]: kSpinLockHeld
//     [1]: kSpinLockCooperative
//     [2]: kSpinLockDisabledScheduling
// [31..3]: ONLY kSpinLockSleeper OR
//          wait time in cycles >> PROFILE_TIMESTAMP_SHIFT
//
// Detailed descriptions:
//
// Bit [0]: The lock is considered held iff kSpinLockHeld is set.
//
// Bit [1]: Eligible waiters (e.g. Fibers) may co-operatively reschedule when
//          contended iff kSpinLockCooperative is set.
//
// Bit [2]: This bit is exclusive from bit [1].  It is used only by a
//          non-cooperative lock.  When set, indicates that scheduling was
//          successfully disabled when the lock was acquired.  May be unset,
//          even if non-cooperative, if a thread_identity did not yet exist at
//          time of acquisition.
//
// Bit [3]: If this is the only upper bit ([31..3]) set then this lock was
//          acquired without contention, however, at least one waiter exists.
//
//          Otherwise, bits [31..3] represent the time spent by the current lock
//          holder to acquire the lock.  There may be outstanding waiter(s).

namespace abel {
    namespace fiber_internal {

        // @s: https://code.woboq.org/userspace/glibc/nptl/pthread_spin_lock.c.html
        void spinlock::lock_slow() noexcept {
            do {
                // Test ...
                while (locked_.load(std::memory_order_relaxed)) {
                    ABEL_CPU_RELAX();
                }

                // ... and set.
            } while (locked_.exchange(true, std::memory_order_acquire));
        }

    }  // namespace fiber_internal

}  // namespace abel
