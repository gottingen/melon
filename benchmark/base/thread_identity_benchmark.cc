//

#include <benchmark/benchmark.h>
#include <abel/base/internal/thread_identity.h>
#include <abel/synchronization/internal/create_thread_identity.h>
#include <abel/synchronization/internal/per_thread_sem.h>

namespace {

void BM_SafeCurrentThreadIdentity(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        abel::synchronization_internal::GetOrCreateCurrentThreadIdentity());
  }
}
BENCHMARK(BM_SafeCurrentThreadIdentity);

void BM_UnsafeCurrentThreadIdentity(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        abel::base_internal::CurrentThreadIdentityIfPresent());
  }
}
BENCHMARK(BM_UnsafeCurrentThreadIdentity);

}  // namespace
