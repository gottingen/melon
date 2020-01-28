//

#include <cstdint>
#include <mutex>  // NOLINT(build/c++11)
#include <vector>

#include <abel/chrono/internal/cycle_clock.h>
#include <abel/threading/internal/spinlock.h>
#include <abel/synchronization/blocking_counter.h>
#include <abel/synchronization/internal/thread_pool.h>
#include <abel/synchronization/mutex.h>
#include <benchmark/benchmark.h>

namespace {

void BM_Mutex(benchmark::State& state) {
  static abel::mutex* mu = new abel::mutex;
  for (auto _ : state) {
    abel::mutex_lock lock(mu);
  }
}
BENCHMARK(BM_Mutex)->UseRealTime()->Threads(1)->ThreadPerCpu();

static void DelayNs(int64_t ns, int* data) {
  int64_t end = abel::chrono_internal::cycle_clock::now() +
                ns * abel::chrono_internal::cycle_clock::frequency() / 1e9;
  while (abel::chrono_internal::cycle_clock::now() < end) {
    ++(*data);
    benchmark::DoNotOptimize(*data);
  }
}

template <typename MutexType>
class RaiiLocker {
 public:
  explicit RaiiLocker(MutexType* mu) : mu_(mu) { mu_->lock(); }
  ~RaiiLocker() { mu_->unlock(); }
 private:
  MutexType* mu_;
};

template <>
class RaiiLocker<std::mutex> {
 public:
  explicit RaiiLocker(std::mutex* mu) : mu_(mu) { mu_->lock(); }
  ~RaiiLocker() { mu_->unlock(); }
 private:
  std::mutex* mu_;
};

template <typename MutexType>
void BM_Contended(benchmark::State& state) {
  struct Shared {
    MutexType mu;
    int data = 0;
  };
  static auto* shared = new Shared;
  int local = 0;
  for (auto _ : state) {
    // Here we model both local work outside of the critical section as well as
    // some work inside of the critical section. The idea is to capture some
    // more or less realisitic contention levels.
    // If contention is too low, the benchmark won't measure anything useful.
    // If contention is unrealistically high, the benchmark will favor
    // bad mutex implementations that block and otherwise distract threads
    // from the mutex and shared state for as much as possible.
    // To achieve this amount of local work is multiplied by number of threads
    // to keep ratio between local work and critical section approximately
    // equal regardless of number of threads.
    DelayNs(100 * state.threads, &local);
    RaiiLocker<MutexType> locker(&shared->mu);
    DelayNs(state.range(0), &shared->data);
  }
}

BENCHMARK_TEMPLATE(BM_Contended, abel::mutex)
    ->UseRealTime()
    // ThreadPerCpu poorly handles non-power-of-two CPU counts.
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(6)
    ->Threads(8)
    ->Threads(12)
    ->Threads(16)
    ->Threads(24)
    ->Threads(32)
    ->Threads(48)
    ->Threads(64)
    ->Threads(96)
    ->Threads(128)
    ->Threads(192)
    ->Threads(256)
    // Some empirically chosen amounts of work in critical section.
    // 1 is low contention, 200 is high contention and few values in between.
    ->Arg(1)
    ->Arg(20)
    ->Arg(50)
    ->Arg(200);

BENCHMARK_TEMPLATE(BM_Contended, abel::threading_internal::SpinLock)
    ->UseRealTime()
    // ThreadPerCpu poorly handles non-power-of-two CPU counts.
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(6)
    ->Threads(8)
    ->Threads(12)
    ->Threads(16)
    ->Threads(24)
    ->Threads(32)
    ->Threads(48)
    ->Threads(64)
    ->Threads(96)
    ->Threads(128)
    ->Threads(192)
    ->Threads(256)
    // Some empirically chosen amounts of work in critical section.
    // 1 is low contention, 200 is high contention and few values in between.
    ->Arg(1)
    ->Arg(20)
    ->Arg(50)
    ->Arg(200);

BENCHMARK_TEMPLATE(BM_Contended, std::mutex)
    ->UseRealTime()
    // ThreadPerCpu poorly handles non-power-of-two CPU counts.
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(6)
    ->Threads(8)
    ->Threads(12)
    ->Threads(16)
    ->Threads(24)
    ->Threads(32)
    ->Threads(48)
    ->Threads(64)
    ->Threads(96)
    ->Threads(128)
    ->Threads(192)
    ->Threads(256)
    // Some empirically chosen amounts of work in critical section.
    // 1 is low contention, 200 is high contention and few values in between.
    ->Arg(1)
    ->Arg(20)
    ->Arg(50)
    ->Arg(200);

// Measure the overhead of conditions on mutex release (when they must be
// evaluated).  mutex has (some) support for equivalence classes allowing
// Conditions with the same function/argument to potentially not be multiply
// evaluated.
//
// num_classes==0 is used for the special case of every waiter being distinct.
void BM_ConditionWaiters(benchmark::State& state) {
  int num_classes = state.range(0);
  int num_waiters = state.range(1);

  struct Helper {
    static void Waiter(abel::blocking_counter* init, abel::mutex* m, int* p) {
      init->decrement_count();
      m->LockWhen(abel::condition(
          static_cast<bool (*)(int*)>([](int* v) { return *v == 0; }), p));
      m->unlock();
    }
  };

  if (num_classes == 0) {
    // No equivalence classes.
    num_classes = num_waiters;
  }

  abel::blocking_counter init(num_waiters);
  abel::mutex mu;
  std::vector<int> equivalence_classes(num_classes, 1);

  // Must be declared last to be destroyed first.
  abel::synchronization_internal::ThreadPool pool(num_waiters);

  for (int i = 0; i < num_waiters; i++) {
    // mutex considers Conditions with the same function and argument
    // to be equivalent.
    pool.Schedule([&, i] {
      Helper::Waiter(&init, &mu, &equivalence_classes[i % num_classes]);
    });
  }
  init.wait();

  for (auto _ : state) {
    mu.lock();
    mu.unlock();  // Each unlock requires condition evaluation for our waiters.
  }

  mu.lock();
  for (int i = 0; i < num_classes; i++) {
    equivalence_classes[i] = 0;
  }
  mu.unlock();
}

// Some configurations have higher thread limits than others.
#if defined(__linux__) && !defined(THREAD_SANITIZER)
constexpr int kMaxConditionWaiters = 8192;
#else
constexpr int kMaxConditionWaiters = 1024;
#endif
BENCHMARK(BM_ConditionWaiters)->RangePair(0, 2, 1, kMaxConditionWaiters);

}  // namespace
