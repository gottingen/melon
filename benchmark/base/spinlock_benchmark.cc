//

// See also //abel/synchronization:mutex_benchmark for a comparison of spin_lock
// and mutex performance under varying levels of contention.

#include <abel/log/abel_logging.h>
#include <abel/thread/internal/scheduling_mode.h>
#include <abel/thread/internal/spinlock.h>
#include <abel/thread/internal/create_thread_identity.h>
#include <benchmark/benchmark.h>

namespace {

    template<abel::thread_internal::SchedulingMode scheduling_mode>
    static void BM_SpinLock(benchmark::State &state) {
        // Ensure a thread_identity is installed.
        ABEL_RAW_CHECK(
                abel::thread_internal::get_or_create_current_thread_identity() !=
                nullptr,
                "get_or_create_current_thread_identity() failed");

        static auto *spinlock = new abel::thread_internal::spin_lock(scheduling_mode);
        for (auto _ : state) {
            abel::thread_internal::SpinLockHolder holder(spinlock);
        }
    }

    BENCHMARK_TEMPLATE(BM_SpinLock,
            abel::thread_internal::SCHEDULE_KERNEL_ONLY
    )
    ->UseRealTime()
    ->Threads(1)
    ->

    ThreadPerCpu();

    BENCHMARK_TEMPLATE(BM_SpinLock,
            abel::thread_internal::SCHEDULE_COOPERATIVE_AND_KERNEL
    )
    ->UseRealTime()
    ->Threads(1)
    ->

    ThreadPerCpu();

}  // namespace
