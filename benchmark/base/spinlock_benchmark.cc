//

// See also //abel/synchronization:mutex_benchmark for a comparison of SpinLock
// and mutex performance under varying levels of contention.

#include <abel/log/raw_logging.h>
#include <abel/threading/internal/scheduling_mode.h>
#include <abel/threading/internal/spinlock.h>
#include <abel/synchronization/internal/create_thread_identity.h>
#include <benchmark/benchmark.h>

namespace {

template <abel::threading_internal::SchedulingMode scheduling_mode>
static void BM_SpinLock(benchmark::State& state) {
  // Ensure a ThreadIdentity is installed.
  ABEL_INTERNAL_CHECK(
      abel::synchronization_internal::GetOrCreateCurrentThreadIdentity() !=
          nullptr,
      "GetOrCreateCurrentThreadIdentity() failed");

  static auto* spinlock = new abel::threading_internal::SpinLock(scheduling_mode);
  for (auto _ : state) {
    abel::threading_internal::SpinLockHolder holder(spinlock);
  }
}

BENCHMARK_TEMPLATE(BM_SpinLock,
                   abel::threading_internal::SCHEDULE_KERNEL_ONLY)
    ->UseRealTime()
    ->Threads(1)
    ->ThreadPerCpu();

BENCHMARK_TEMPLATE(BM_SpinLock,
                   abel::threading_internal::SCHEDULE_COOPERATIVE_AND_KERNEL)
    ->UseRealTime()
    ->Threads(1)
    ->ThreadPerCpu();

}  // namespace
