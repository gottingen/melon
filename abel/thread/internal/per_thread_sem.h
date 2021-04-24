
// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//
// per_thread_sem is a low-level synchronization primitive controlling the
// runnability of a single thread, used internally by mutex and cond_var.
//
// This is NOT a general-purpose synchronization mechanism, and should not be
// used directly by applications.  Applications should use mutex and cond_var.
//
// The semantics of per_thread_sem are the same as that of a counting semaphore.
// Each thread maintains an abstract "count" value associated with its identity.

#ifndef ABEL_SYNCHRONIZATION_INTERNAL_PER_THREAD_SEM_H_
#define ABEL_SYNCHRONIZATION_INTERNAL_PER_THREAD_SEM_H_

#include <atomic>

#include "abel/thread/internal/thread_identity.h"
#include "abel/thread/internal/create_thread_identity.h"
#include "abel/thread/internal/kernel_timeout.h"

namespace abel {


class mutex;

namespace thread_internal {

class per_thread_sem {
  public:
    per_thread_sem() = delete;

    per_thread_sem(const per_thread_sem &) = delete;

    per_thread_sem &operator=(const per_thread_sem &) = delete;

    // Routine invoked periodically (once a second) by a background thread.
    // Has no effect on user-visible state.
    static void tick(thread_internal::thread_identity *identity);

    // ---------------------------------------------------------------------------
    // Routines used by autosizing threadpools to detect when threads are
    // blocked.  Each thread has a counter pointer, initially zero.  If non-zero,
    // the implementation atomically increments the counter when it blocks on a
    // semaphore, a decrements it again when it wakes.  This allows a threadpool
    // to keep track of how many of its threads are blocked.
    // set_thread_blocked_counter() should be used only by threadpool
    // implementations.  get_thread_blocked_counter() should be used by modules that
    // block threads; if the pointer returned is non-zero, the location should be
    // incremented before the thread blocks, and decremented after it wakes.
    static void set_thread_blocked_counter(std::atomic<int> *counter);

    static std::atomic<int> *get_thread_blocked_counter();

  private:
    // Create the per_thread_sem associated with "identity".  Initializes count=0.
    // REQUIRES: May only be called by thread_identity.
    static void init(thread_internal::thread_identity *identity);

    // Destroy the per_thread_sem associated with "identity".
    // REQUIRES: May only be called by thread_identity.
    static void destroy(thread_internal::thread_identity *identity);

    // Increments "identity"'s count.
    static ABEL_FORCE_INLINE void post(thread_internal::thread_identity *identity);

    // Waits until either our count > 0 or t has expired.
    // If count > 0, decrements count and returns true.  Otherwise returns false.
    // !t.has_timeout() => wait(t) will return true.
    static ABEL_FORCE_INLINE bool wait(kernel_timeout t);

    // White-listed callers.
    friend class PerThreadSemTest;

    friend class abel::mutex;

    friend abel::thread_internal::thread_identity *create_thread_identity();

    friend void reclaim_thread_identity(void *v);
};

}  // namespace thread_internal

}  // namespace abel

// In some build configurations we pass --detect-odr-violations to the
// gold linker.  This causes it to flag weak symbol overrides as ODR
// violations.  Because ODR only applies to C++ and not C,
// --detect-odr-violations ignores symbols not mangled with C++ names.
// By changing our extension points to be extern "C", we dodge this
// check.
extern "C" {
void AbelInternalPerThreadSemPost(
        abel::thread_internal::thread_identity *identity);
bool AbelInternalPerThreadSemWait(
        abel::thread_internal::kernel_timeout t);
}  // extern "C"

void abel::thread_internal::per_thread_sem::post(
        abel::thread_internal::thread_identity *identity) {
    AbelInternalPerThreadSemPost(identity);
}

bool abel::thread_internal::per_thread_sem::wait(
        abel::thread_internal::kernel_timeout t) {
    return AbelInternalPerThreadSemWait(t);
}

#endif  // ABEL_SYNCHRONIZATION_INTERNAL_PER_THREAD_SEM_H_
