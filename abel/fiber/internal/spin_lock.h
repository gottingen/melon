// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//
//  Most users requiring mutual exclusion should use mutex.
//  spin_lock is provided for use in three situations:
//   - for use in code that mutex itself depends on
//   - to get a faster fast-path release under low contention (without an
//     atomic read-modify-write) In return, spin_lock has worse behaviour under
//     contention, which is why mutex is preferred in most situations.
//   - for async signal safety (see below)

// spin_lock is async signal safe.  If a spinlock is used within a signal
// handler, all code that acquires the lock must ensure that the signal cannot
// arrive while they are holding the lock.  Typically, this is done by blocking
// the signal.

#ifndef ABEL_THREADING_INTERNAL_SPINLOCK_H_
#define ABEL_THREADING_INTERNAL_SPINLOCK_H_

#include <cstdint>
#include <sys/types.h>
#include <atomic>

#include "abel/base/profile.h"

namespace abel {

namespace fiber_internal {
    // The class' name explains itself well.
    //
    // FIXME: Do we need TSan annotations here?
    class spinlock {
    public:
        void lock() noexcept {
            // Here we try to grab the lock first before failing back to TTAS.
            //
            // If the lock is not contend, this fast-path should be quicker.
            // If the lock is contended and we have to fail back to slow TTAS, this
            // single try shouldn't add too much overhead.
            //
            // What's more, by keeping this method small, chances are higher that this
            // method get inlined by the compiler.
            if (ABEL_LIKELY(try_lock())) {
                return;
            }

            // Slow path otherwise.
            lock_slow();
        }

        bool try_lock() noexcept {
            return !locked_.exchange(true, std::memory_order_acquire);
        }

        void unlock() noexcept { locked_.store(false, std::memory_order_release); }

    private:
        void lock_slow() noexcept;

    private:
        std::atomic<bool> locked_{false};
    };

} // namespace fiber_internal
}  // namespace abel

#endif  // ABEL_THREADING_INTERNAL_SPINLOCK_H_
