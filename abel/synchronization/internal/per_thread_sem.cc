//

// This file is a no-op if the required LowLevelAlloc support is missing.
#include <abel/memory/internal/low_level_alloc.h>
#ifndef ABEL_LOW_LEVEL_ALLOC_MISSING

#include <abel/synchronization/internal/per_thread_sem.h>

#include <atomic>

#include <abel/base/profile.h>
#include <abel/threading/internal/thread_identity.h>
#include <abel/synchronization/internal/waiter.h>

namespace abel {

namespace synchronization_internal {

void PerThreadSem::SetThreadBlockedCounter (std::atomic<int> *counter) {
    threading_internal::ThreadIdentity *identity;
    identity = GetOrCreateCurrentThreadIdentity();
    identity->blocked_count_ptr = counter;
}

std::atomic<int> *PerThreadSem::GetThreadBlockedCounter () {
    threading_internal::ThreadIdentity *identity;
    identity = GetOrCreateCurrentThreadIdentity();
    return identity->blocked_count_ptr;
}

void PerThreadSem::Init (threading_internal::ThreadIdentity *identity) {
    new(Waiter::GetWaiter(identity)) Waiter();
    identity->ticker.store(0, std::memory_order_relaxed);
    identity->wait_start.store(0, std::memory_order_relaxed);
    identity->is_idle.store(false, std::memory_order_relaxed);
}

void PerThreadSem::Destroy (threading_internal::ThreadIdentity *identity) {
    Waiter::GetWaiter(identity)->~Waiter();
}

void PerThreadSem::Tick (threading_internal::ThreadIdentity *identity) {
    const int ticker =
        identity->ticker.fetch_add(1, std::memory_order_relaxed) + 1;
    const int wait_start = identity->wait_start.load(std::memory_order_relaxed);
    const bool is_idle = identity->is_idle.load(std::memory_order_relaxed);
    if (wait_start && (ticker - wait_start > Waiter::kIdlePeriods) && !is_idle) {
        // Wakeup the waiting thread since it is time for it to become idle.
        Waiter::GetWaiter(identity)->Poke();
    }
}

}  // namespace synchronization_internal

}  // namespace abel

extern "C" {

ABEL_WEAK void AbelInternalPerThreadSemPost (
    abel::threading_internal::ThreadIdentity *identity) {
    abel::synchronization_internal::Waiter::GetWaiter(identity)->Post();
}

ABEL_WEAK bool AbelInternalPerThreadSemWait (
    abel::synchronization_internal::KernelTimeout t) {
    bool timeout = false;
    abel::threading_internal::ThreadIdentity *identity;
    identity = abel::synchronization_internal::GetOrCreateCurrentThreadIdentity();

    // Ensure wait_start != 0.
    int ticker = identity->ticker.load(std::memory_order_relaxed);
    identity->wait_start.store(ticker ? ticker : 1, std::memory_order_relaxed);
    identity->is_idle.store(false, std::memory_order_relaxed);

    if (identity->blocked_count_ptr != nullptr) {
        // Increment count of threads blocked in a given thread pool.
        identity->blocked_count_ptr->fetch_add(1, std::memory_order_relaxed);
    }

    timeout =
        !abel::synchronization_internal::Waiter::GetWaiter(identity)->wait(t);

    if (identity->blocked_count_ptr != nullptr) {
        identity->blocked_count_ptr->fetch_sub(1, std::memory_order_relaxed);
    }

    identity->is_idle.store(false, std::memory_order_relaxed);
    identity->wait_start.store(0, std::memory_order_relaxed);
    return !timeout;
}

}  // extern "C"

#endif  // ABEL_LOW_LEVEL_ALLOC_MISSING
