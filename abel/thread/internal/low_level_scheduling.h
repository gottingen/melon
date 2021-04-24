// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//
// Core interfaces and definitions used by by low-level interfaces such as
// spin_lock.

#ifndef ABEL_THREADING_INTERNAL_LOW_LEVEL_SCHEDULING_H_
#define ABEL_THREADING_INTERNAL_LOW_LEVEL_SCHEDULING_H_

#include "abel/thread/internal/scheduling_mode.h"
#include "abel/base/profile.h"

// The following two declarations exist so scheduling_guard may friend them with
// the appropriate language linkage.  These callbacks allow libc internals, such
// as function level statics, to schedule cooperatively when locking.
extern "C" bool __google_disable_rescheduling(void);
extern "C" void __google_enable_rescheduling(bool disable_result);

namespace abel {

namespace base_internal {
class scheduling_helper;  // To allow use of scheduling_guard.

}
// forward declare
class spin_lock;

namespace thread_internal {

// scheduling_guard
// Provides guard semantics that may be used to disable cooperative rescheduling
// of the calling thread within specific program blocks.  This is used to
// protect resources (e.g. low-level SpinLocks or Domain code) that cooperative
// scheduling depends on.
//
// Domain implementations capable of rescheduling in reaction to involuntary
// kernel thread actions (e.g blocking due to a pagefault or syscall) must
// guarantee that an annotated thread is not allowed to (cooperatively)
// reschedule until the annotated region is complete.
//
// It is an error to attempt to use a cooperatively scheduled resource (e.g.
// mutex) within a rescheduling-disabled region.
//
// All methods are async-signal safe.
class scheduling_guard {
  public:
    // Returns true iff the calling thread may be cooperatively rescheduled.
    static bool rescheduling_is_allowed();

  private:
    // Disable cooperative rescheduling of the calling thread.  It may still
    // initiate scheduling operations (e.g. wake-ups), however, it may not itself
    // reschedule.  Nestable.  The returned result is opaque, clients should not
    // attempt to interpret it.
    // REQUIRES: Result must be passed to a pairing EnableScheduling().
    static bool disable_rescheduling();

    // Marks the end of a rescheduling disabled region, previously started by
    // disable_rescheduling().
    // REQUIRES: Pairs with innermost call (and result) of disable_rescheduling().
    static void enable_rescheduling(bool disable_result);

    // A scoped helper for {Disable, Enable}Rescheduling().
    // REQUIRES: destructor must run in same thread as constructor.
    struct scoped_disable {
        scoped_disable() { disabled = scheduling_guard::disable_rescheduling(); }

        ~scoped_disable() { scheduling_guard::enable_rescheduling(disabled); }

        bool disabled;
    };

    // Access to scheduling_guard is explicitly white-listed.
    friend class abel::base_internal::scheduling_helper;

    friend class abel::spin_lock;

    scheduling_guard(const scheduling_guard &) = delete;

    scheduling_guard &operator=(const scheduling_guard &) = delete;
};

//------------------------------------------------------------------------------
// End of public interfaces.
//------------------------------------------------------------------------------

ABEL_FORCE_INLINE bool scheduling_guard::rescheduling_is_allowed() {
    return false;
}

ABEL_FORCE_INLINE bool scheduling_guard::disable_rescheduling() {
    return false;
}

ABEL_FORCE_INLINE void scheduling_guard::enable_rescheduling(bool /* disable_result */) {
    return;
}

}  // namespace thread_internal

}  // namespace abel

#endif  // ABEL_BASE_INTERNAL_LOW_LEVEL_SCHEDULING_H_
