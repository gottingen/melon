// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

// This file is a no-op if the required low_level_alloc support is missing.
#include "abel/memory/internal/low_level_alloc.h"

#ifndef ABEL_LOW_LEVEL_ALLOC_MISSING

#include "abel/thread/internal/per_thread_sem.h"

#include <atomic>

#include "abel/base/profile.h"
#include "abel/thread/internal/thread_identity.h"
#include "abel/thread/internal/waiter.h"

namespace abel {

namespace thread_internal {

void per_thread_sem::set_thread_blocked_counter(std::atomic<int> *counter) {
    thread_internal::thread_identity *identity;
    identity = get_or_create_current_thread_identity();
    identity->blocked_count_ptr = counter;
}

std::atomic<int> *per_thread_sem::get_thread_blocked_counter() {
    thread_internal::thread_identity *identity;
    identity = get_or_create_current_thread_identity();
    return identity->blocked_count_ptr;
}

void per_thread_sem::init(thread_internal::thread_identity *identity) {
    new(waiter::get_waiter(identity)) waiter();
    identity->ticker.store(0, std::memory_order_relaxed);
    identity->wait_start.store(0, std::memory_order_relaxed);
    identity->is_idle.store(false, std::memory_order_relaxed);
}

void per_thread_sem::destroy(thread_internal::thread_identity *identity) {
    waiter::get_waiter(identity)->~waiter();
}

void per_thread_sem::tick(thread_internal::thread_identity *identity) {
    const int ticker =
            identity->ticker.fetch_add(1, std::memory_order_relaxed) + 1;
    const int wait_start = identity->wait_start.load(std::memory_order_relaxed);
    const bool is_idle = identity->is_idle.load(std::memory_order_relaxed);
    if (wait_start && (ticker - wait_start > waiter::kIdlePeriods) && !is_idle) {
        // wakeup the waiting thread since it is time for it to become idle.
        waiter::get_waiter(identity)->poke();
    }
}

}  // namespace thread_internal

}  // namespace abel

extern "C" {

ABEL_WEAK void AbelInternalPerThreadSemPost(
        abel::thread_internal::thread_identity *identity) {
    abel::thread_internal::waiter::get_waiter(identity)->post();
}

ABEL_WEAK bool AbelInternalPerThreadSemWait(
        abel::thread_internal::kernel_timeout t) {
    bool timeout = false;
    abel::thread_internal::thread_identity *identity;
    identity = abel::thread_internal::get_or_create_current_thread_identity();

    // Ensure wait_start != 0.
    int ticker = identity->ticker.load(std::memory_order_relaxed);
    identity->wait_start.store(ticker ? ticker : 1, std::memory_order_relaxed);
    identity->is_idle.store(false, std::memory_order_relaxed);

    if (identity->blocked_count_ptr != nullptr) {
        // Increment count of threads blocked in a given thread pool.
        identity->blocked_count_ptr->fetch_add(1, std::memory_order_relaxed);
    }

    timeout =
            !abel::thread_internal::waiter::get_waiter(identity)->wait(t);

    if (identity->blocked_count_ptr != nullptr) {
        identity->blocked_count_ptr->fetch_sub(1, std::memory_order_relaxed);
    }

    identity->is_idle.store(false, std::memory_order_relaxed);
    identity->wait_start.store(0, std::memory_order_relaxed);
    return !timeout;
}

}  // extern "C"

#endif  // ABEL_LOW_LEVEL_ALLOC_MISSING
