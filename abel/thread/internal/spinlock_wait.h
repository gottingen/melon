// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_BASE_INTERNAL_SPINLOCK_WAIT_H_
#define ABEL_BASE_INTERNAL_SPINLOCK_WAIT_H_

// Operations to make atomic transitions on a word, and to allow
// waiting for those transitions to become possible.

#include <cstdint>
#include <atomic>

#include "abel/thread/internal/scheduling_mode.h"

namespace abel {

namespace thread_internal {

// spin_lock_wait() waits until it can perform one of several transitions from
// "from" to "to".  It returns when it performs a transition where done==true.
struct spin_lock_wait_transition {
    uint32_t from;
    uint32_t to;
    bool done;
};

// wait until *w can transition from trans[i].from to trans[i].to for some i
// satisfying 0<=i<n && trans[i].done, atomically make the transition,
// then return the old value of *w.   Make any other atomic transitions
// where !trans[i].done, but continue waiting.
uint32_t spin_lock_wait(std::atomic<uint32_t> *w, int n,
                        const spin_lock_wait_transition trans[],
                        thread_internal::SchedulingMode scheduling_mode);

// If possible, wake some thread that has called spin_lock_delay(w, ...). If
// "all" is true, wake all such threads.  This call is a hint, and on some
// systems it may be a no-op; threads calling spin_lock_delay() will always wake
// eventually even if spin_lock_wake() is never called.
void spin_lock_wake(std::atomic<uint32_t> *w, bool all);

// wait for an appropriate spin delay on iteration "loop" of a
// spin loop on location *w, whose previously observed value was "value".
// spin_lock_delay() may do nothing, may yield the CPU, may sleep a clock tick,
// or may wait for a delay that can be truncated by a call to spin_lock_wake(w).
// In all cases, it must return in bounded time even if spin_lock_wake() is not
// called.
void spin_lock_delay(std::atomic<uint32_t> *w, uint32_t value, int loop,
                     thread_internal::SchedulingMode scheduling_mode);

// Helper used by abel_internal_spin_lock_delay.
// Returns a suggested delay in nanoseconds for iteration number "loop".
int spin_lock_suggested_delay_ns(int loop);

}  // namespace thread_internal

}  // namespace abel

// In some build configurations we pass --detect-odr-violations to the
// gold linker.  This causes it to flag weak symbol overrides as ODR
// violations.  Because ODR only applies to C++ and not C,
// --detect-odr-violations ignores symbols not mangled with C++ names.
// By changing our extension points to be extern "C", we dodge this
// check.
extern "C" {
void abel_internal_spin_lock_wake(std::atomic<uint32_t> *w, bool all);
void abel_internal_spin_lock_delay(
        std::atomic<uint32_t> *w, uint32_t value, int loop,
        abel::thread_internal::SchedulingMode scheduling_mode);
}

ABEL_FORCE_INLINE void abel::thread_internal::spin_lock_wake(std::atomic<uint32_t> *w,
                                                             bool all) {
    abel_internal_spin_lock_wake(w, all);
}

ABEL_FORCE_INLINE void abel::thread_internal::spin_lock_delay(
        std::atomic<uint32_t> *w, uint32_t value, int loop,
        abel::thread_internal::SchedulingMode scheduling_mode) {
    abel_internal_spin_lock_delay(w, value, loop, scheduling_mode);
}

#endif  // ABEL_BASE_INTERNAL_SPINLOCK_WAIT_H_
