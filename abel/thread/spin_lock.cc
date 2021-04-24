// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/thread/spin_lock.h"

#include <algorithm>
#include <atomic>
#include <limits>

#include "abel/base/profile.h"
#include "abel/atomic/atomic_hook.h"
#include "abel/chrono/internal/cycle_clock.h"
#include "abel/thread/internal/spinlock_wait.h"
#include "abel/system/sysinfo.h" /* For num_cpus() */
#include "abel/thread/call_once.h"

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


ABEL_CONST_INIT static atomic_hook<void (*)(const void *lock,
                                            int64_t wait_cycles)>
        submit_profile_data;

void register_spin_lock_profiler(void (*fn)(const void *contendedlock,
                                            int64_t wait_cycles)) {
    submit_profile_data.Store(fn);
}

// Uncommon constructors.
spin_lock::spin_lock(thread_internal::SchedulingMode mode)
        : lockword_(IsCooperative(mode) ? kSpinLockCooperative : 0) {
    ABEL_TSAN_MUTEX_CREATE(this, __tsan_mutex_not_static);
}

spin_lock::spin_lock(base_internal::LinkerInitialized,
                     thread_internal::SchedulingMode mode) {
    ABEL_TSAN_MUTEX_CREATE(this, 0);
    if (IsCooperative(mode)) {
        init_linker_initialized_and_cooperative();
    }
    // Otherwise, lockword_ is already initialized.
}

// Static (linker initialized) spinlocks always start life as functional
// non-cooperative locks.  When their static constructor does run, it will call
// this initializer to augment the lockword with the cooperative bit.  By
// actually taking the lock when we do this we avoid the need for an atomic
// operation in the regular unlock path.
//
// slow_lock() must be careful to re-test for this bit so that any outstanding
// waiters may be upgraded to cooperative status.
void spin_lock::init_linker_initialized_and_cooperative() {
    lock();
    lockword_.fetch_or(kSpinLockCooperative, std::memory_order_relaxed);
    unlock();
}

// Monitor the lock to see if its value changes within some time period
// (adaptive_spin_count loop iterations). The last value read from the lock
// is returned from the method.
uint32_t spin_lock::spin_loop() {
    // We are already in the slow path of spin_lock, initialize the
    // adaptive_spin_count here.
    ABEL_CONST_INIT static abel::once_flag init_adaptive_spin_count;
    ABEL_CONST_INIT static int adaptive_spin_count = 0;
    base_internal::low_level_call_once(&init_adaptive_spin_count, []() {
        adaptive_spin_count = num_cpus() > 1 ? 1000 : 1;
    });

    int c = adaptive_spin_count;
    uint32_t lock_value;
    do {
        lock_value = lockword_.load(std::memory_order_relaxed);
    } while ((lock_value & kSpinLockHeld) != 0 && --c > 0);
    return lock_value;
}

void spin_lock::slow_lock() {
    uint32_t lock_value = spin_loop();
    lock_value = try_lock_internal(lock_value, 0);
    if ((lock_value & kSpinLockHeld) == 0) {
        return;
    }
    // The lock was not obtained initially, so this thread needs to wait for
    // it.  Record the current timestamp in the local variable wait_start_time
    // so the total wait time can be stored in the lockword once this thread
    // obtains the lock.
    int64_t wait_start_time = abel::chrono_internal::cycle_clock::now();
    uint32_t wait_cycles = 0;
    int lock_wait_call_count = 0;
    while ((lock_value & kSpinLockHeld) != 0) {
        // If the lock is currently held, but not marked as having a sleeper, mark
        // it as having a sleeper.
        if ((lock_value & kWaitTimeMask) == 0) {
            // Here, just "mark" that the thread is going to sleep.  Don't store the
            // lock wait time in the lock as that will cause the current lock
            // owner to think it experienced contention.
            if (lockword_.compare_exchange_strong(
                    lock_value, lock_value | kSpinLockSleeper,
                    std::memory_order_relaxed, std::memory_order_relaxed)) {
                // Successfully transitioned to kSpinLockSleeper.  Pass
                // kSpinLockSleeper to the spin_lock_wait routine to properly indicate
                // the last lock_value observed.
                lock_value |= kSpinLockSleeper;
            } else if ((lock_value & kSpinLockHeld) == 0) {
                // lock is free again, so try and acquire it before sleeping.  The
                // new lock state will be the number of cycles this thread waited if
                // this thread obtains the lock.
                lock_value = try_lock_internal(lock_value, wait_cycles);
                continue;   // Skip the delay at the end of the loop.
            }
        }

        thread_internal::SchedulingMode scheduling_mode;
        if ((lock_value & kSpinLockCooperative) != 0) {
            scheduling_mode = thread_internal::SCHEDULE_COOPERATIVE_AND_KERNEL;
        } else {
            scheduling_mode = thread_internal::SCHEDULE_KERNEL_ONLY;
        }
        // spin_lock_delay() calls into fiber scheduler, we need to see
        // synchronization there to avoid false positives.
        ABEL_TSAN_MUTEX_PRE_DIVERT(this, 0);
        // wait for an OS specific delay.
        thread_internal::spin_lock_delay(&lockword_, lock_value, ++lock_wait_call_count,
                                         scheduling_mode);
        ABEL_TSAN_MUTEX_POST_DIVERT(this, 0);
        // Spin again after returning from the wait routine to give this thread
        // some chance of obtaining the lock.
        lock_value = spin_loop();
        wait_cycles = encode_wait_cycles(wait_start_time, chrono_internal::cycle_clock::now());
        lock_value = try_lock_internal(lock_value, wait_cycles);
    }
}

void spin_lock::slow_unlock(uint32_t lock_value) {
    thread_internal::spin_lock_wake(&lockword_,
                                    false);  // wake waiter if necessary

    // If our acquisition was contended, collect contentionz profile info.  We
    // reserve a unitary wait time to represent that a waiter exists without our
    // own acquisition having been contended.
    if ((lock_value & kWaitTimeMask) != kSpinLockSleeper) {
        const uint64_t wait_cycles = decode_wait_cycles(lock_value);
        ABEL_TSAN_MUTEX_PRE_DIVERT(this, 0);
        submit_profile_data(this, wait_cycles);
        ABEL_TSAN_MUTEX_POST_DIVERT(this, 0);
    }
}

// We use the upper 29 bits of the lock word to store the time spent waiting to
// acquire this lock.  This is reported by contentionz profiling.  Since the
// lower bits of the cycle counter wrap very quickly on high-frequency
// processors we divide to reduce the granularity to 2^PROFILE_TIMESTAMP_SHIFT
// sized units.  On a 4Ghz machine this will lose track of wait times greater
// than (2^29/4 Ghz)*128 =~ 17.2 seconds.  Such waits should be extremely rare.
enum {
    PROFILE_TIMESTAMP_SHIFT = 7
};
enum {
    LOCKWORD_RESERVED_SHIFT = 3
};  // We currently reserve the lower 3 bits.

uint32_t spin_lock::encode_wait_cycles(int64_t wait_start_time,
                                       int64_t wait_end_time) {
    static const int64_t kMaxWaitTime =
            std::numeric_limits<uint32_t>::max() >> LOCKWORD_RESERVED_SHIFT;
    int64_t scaled_wait_time =
            (wait_end_time - wait_start_time) >> PROFILE_TIMESTAMP_SHIFT;

    // Return a representation of the time spent waiting that can be stored in
    // the lock word's upper bits.
    uint32_t clamped = static_cast<uint32_t>(
            std::min(scaled_wait_time, kMaxWaitTime) << LOCKWORD_RESERVED_SHIFT);

    if (clamped == 0) {
        return kSpinLockSleeper;  // Just wake waiters, but don't record contention.
    }
    // Bump up value if necessary to avoid returning kSpinLockSleeper.
    const uint32_t kMinWaitTime =
            kSpinLockSleeper + (1 << LOCKWORD_RESERVED_SHIFT);
    if (clamped == kSpinLockSleeper) {
        return kMinWaitTime;
    }
    return clamped;
}

uint64_t spin_lock::decode_wait_cycles(uint32_t lock_value) {
    // Cast to uint32_t first to ensure bits [63:32] are cleared.
    const uint64_t scaled_wait_time =
            static_cast<uint32_t>(lock_value & kWaitTimeMask);
    return scaled_wait_time
            << (PROFILE_TIMESTAMP_SHIFT - LOCKWORD_RESERVED_SHIFT);
}

}  // namespace abel
