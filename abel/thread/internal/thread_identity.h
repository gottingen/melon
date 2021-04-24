// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//
// Each active thread has an thread_identity that may represent the thread in
// various level interfaces.  thread_identity objects are never deallocated.
// When a thread terminates, its thread_identity object may be reused for a
// thread created later.

#ifndef ABEL_BASE_INTERNAL_THREAD_IDENTITY_H_
#define ABEL_BASE_INTERNAL_THREAD_IDENTITY_H_

#ifndef _WIN32

#include <pthread.h>
// Defines __GOOGLE_GRTE_VERSION__ (via glibc-specific features.h) when
// supported.
#include <unistd.h>

#endif

#include <atomic>
#include <cstdint>
#include "abel/base/profile.h"

namespace abel {


struct synch_locks_held;
struct synch_wait_params;

namespace thread_internal {

class spin_lock;

struct thread_identity;

// Used by the implementation of abel::mutex and abel::cond_var.
struct per_thread_synch {
    // The internal representation of abel::mutex and abel::cond_var rely
    // on the alignment of per_thread_synch. Both store the address of the
    // per_thread_synch in the high-order bits of their internal state,
    // which means the low kLowZeroBits of the address of per_thread_synch
    // must be zero.
    static constexpr int kLowZeroBits = 8;
    static constexpr int kAlignment = 1 << kLowZeroBits;

    // Returns the associated thread_identity.
    // This can be implemented as a cast because we guarantee
    // per_thread_synch is the first element of thread_identity.
    struct thread_identity *thread_identity() {
        return reinterpret_cast<struct thread_identity *>(this);
    }

    per_thread_synch *next;  // Circular waiter queue; initialized to 0.
    per_thread_synch *skip;  // If non-zero, all entries in mutex queue
    // up to and including "skip" have same
    // condition as this, and will be woken later
    bool may_skip;         // if false while on mutex queue, a mutex unlocker
    // is using this per_thread_synch as a terminator.  Its
    // skip field must not be filled in because the loop
    // might then skip over the terminator.

    // The wait parameters of the current wait.  waitp is null if the
    // thread is not waiting. Transitions from null to non-null must
    // occur before the enqueue commit point (state = kQueued in
    // Enqueue() and CondVarEnqueue()). Transitions from non-null to
    // null must occur after the wait is finished (state = kAvailable in
    // mutex::block() and cond_var::WaitCommon()). This field may be
    // changed only by the thread that describes this per_thread_synch.  A
    // special case is fer(), which calls Enqueue() on another thread,
    // but with an identical synch_wait_params pointer, thus leaving the
    // pointer unchanged.
    synch_wait_params *waitp;

    bool suppress_fatal_errors;  // If true, try to proceed even in the face of
    // broken invariants.  This is used within fatal
    // signal handlers to improve the chances of
    // debug logging information being output
    // successfully.

    intptr_t readers;     // Number of readers in mutex.
    int priority;         // Priority of thread (updated every so often).

    // When priority will next be read (cycles).
    int64_t next_priority_read_cycles;

    // State values:
    //   kAvailable: This per_thread_synch is available.
    //   kQueued: This per_thread_synch is unavailable, it's currently queued on a
    //            mutex or cond_var waistlist.
    //
    // Transitions from kQueued to kAvailable require a release
    // barrier. This is needed as a waiter may use "state" to
    // independently observe that it's no longer queued.
    //
    // Transitions from kAvailable to kQueued require no barrier, they
    // are externally ordered by the mutex.
    enum State {
        kAvailable,
        kQueued
    };
    std::atomic<State> state;

    bool maybe_unlocking;  // Valid at head of mutex waiter queue;
    // true if unlock_slow could be searching
    // for a waiter to wake.  Used for an optimization
    // in Enqueue().  true is always a valid value.
    // Can be reset to false when the unlocker or any
    // writer releases the lock, or a reader fully releases
    // the lock.  It may not be set to false by a reader
    // that decrements the count to non-zero.
    // protected by mutex spinlock

    bool wake;  // This thread is to be woken from a mutex.

    // If "x" is on a waiter list for a mutex, "x->cond_waiter" is true iff the
    // waiter is waiting on the mutex as part of a CV wait or mutex await.
    //
    // The value of "x->cond_waiter" is meaningless if "x" is not on a
    // mutex waiter list.
    bool cond_waiter;

    // Locks held; used during deadlock detection.
    // Allocated in Synch_GetAllLocks() and freed in reclaim_thread_identity().
    synch_locks_held *all_locks;
};

struct thread_identity {
    // Must be the first member.  The mutex implementation requires that
    // the per_thread_synch object associated with each thread is
    // per_thread_synch::kAlignment aligned.  We provide this alignment on
    // thread_identity itself.
    struct per_thread_synch per_thread_synch;

    // Private: Reserved for abel::thread_internal::waiter.
    struct waiter_state {
        char data[128];
    } waiter_state;

    // Used by per_thread_sem::{Get,Set}ThreadBlockedCounter().
    std::atomic<int> *blocked_count_ptr;

    // The following variables are mostly read/written just by the
    // thread itself.  The only exception is that these are read by
    // a ticker thread as a hint.
    std::atomic<int> ticker;      // tick counter, incremented once per second.
    std::atomic<int> wait_start;  // Ticker value when thread started waiting.
    std::atomic<bool> is_idle;    // Has thread become idle yet?

    thread_identity *next;
};

// Returns the thread_identity object representing the calling thread; guaranteed
// to be unique for its lifetime.  The returned object will remain valid for the
// program's lifetime; although it may be re-assigned to a subsequent thread.
// If one does not exist, return nullptr instead.
//
// Does not malloc(*), and is async-signal safe.
// [*] Technically pthread_setspecific() does malloc on first use; however this
// is handled internally within tcmalloc's initialization already.
//
// New thread_identity objects can be constructed and associated with a thread
// by calling get_or_create_current_thread_identity() in per-thread-sem.h.
thread_identity *current_thread_identity_if_present();

using thread_identity_reclaimer_function = void (*)(void *);

// Sets the current thread identity to the given value.  'reclaimer' is a
// pointer to the global function for cleaning up instances on thread
// destruction.
void set_current_thread_identity(thread_identity *identity,
                                 thread_identity_reclaimer_function reclaimer);

// Removes the currently associated thread_identity from the running thread.
// This must be called from inside the thread_identity_reclaimer_function, and only
// from that function.
void clear_current_thread_identity();


ABEL_CONST_INIT extern ABEL_THREAD_LOCAL thread_identity *
        thread_identity_ptr;

ABEL_FORCE_INLINE thread_identity *current_thread_identity_if_present() {
    return thread_identity_ptr;
}


}  // namespace thread_internal

}  // namespace abel

#endif  // ABEL_BASE_INTERNAL_THREAD_IDENTITY_H_
