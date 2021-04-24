// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

// Core interfaces and definitions used by by low-level interfaces such as
// spin_lock.

#ifndef ABEL_THREADING_INTERNAL_SCHEDULING_MODE_H_
#define ABEL_THREADING_INTERNAL_SCHEDULING_MODE_H_

#include "abel/base/profile.h"

namespace abel {

namespace thread_internal {

// Used to describe how a thread may be scheduled.  Typically associated with
// the declaration of a resource supporting synchronized access.
//
// SCHEDULE_COOPERATIVE_AND_KERNEL:
// Specifies that when waiting, a cooperative thread (e.g. a Fiber) may
// reschedule (using base::scheduling semantics); allowing other cooperative
// threads to proceed.
//
// SCHEDULE_KERNEL_ONLY: (Also described as "non-cooperative")
// Specifies that no cooperative scheduling semantics may be used, even if the
// current thread is itself cooperatively scheduled.  This means that
// cooperative threads will NOT allow other cooperative threads to execute in
// their place while waiting for a resource of this type.  Host operating system
// semantics (e.g. a futex) may still be used.
//
// When optional, clients should strongly prefer SCHEDULE_COOPERATIVE_AND_KERNEL
// by default.  SCHEDULE_KERNEL_ONLY should only be used for resources on which
// base::scheduling (e.g. the implementation of a Scheduler) may depend.
//
// NOTE: Cooperative resources may not be nested below non-cooperative ones.
// This means that it is invalid to to acquire a SCHEDULE_COOPERATIVE_AND_KERNEL
// resource if a SCHEDULE_KERNEL_ONLY resource is already held.
enum SchedulingMode {
    SCHEDULE_KERNEL_ONLY = 0,         // Allow scheduling only the host OS.
    SCHEDULE_COOPERATIVE_AND_KERNEL,  // Also allow cooperative scheduling.
};

}  // namespace thread_internal

}  // namespace abel

#endif  // ABEL_THREADING_INTERNAL_SCHEDULING_MODE_H_
