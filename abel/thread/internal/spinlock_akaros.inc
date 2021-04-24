// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//
// This file is an Akaros-specific part of spinlock_wait.cc

#include <atomic>

#include "abel/thread/internal/scheduling_mode.h"

extern "C" {

ABEL_WEAK void abel_internal_spin_lock_delay(
        std::atomic<uint32_t> * /* lock_word */, uint32_t /* value */,
        int /* loop */, abel::thread_internal::SchedulingMode /* mode */) {
    // In Akaros, one must take care not to call anything that could cause a
    // malloc(), a blocking system call, or a uthread_yield() while holding a
    // spinlock. Our callers assume will not call into libraries or other
    // arbitrary code.
}

ABEL_WEAK void abel_internal_spin_lock_wake(
        std::atomic<uint32_t> * /* lock_word */, bool /* all */) {}

}  // extern "C"
