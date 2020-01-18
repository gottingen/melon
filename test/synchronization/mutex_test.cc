//

#include <abel/synchronization/mutex.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <gtest/gtest.h>
#include <abel/base/profile.h>
#include <abel/base/internal/raw_logging.h>
#include <abel/base/internal/sysinfo.h>
#include <abel/memory/memory.h>
#include <abel/synchronization/internal/thread_pool.h>
#include <abel/time/clock.h>
#include <abel/time/time.h>

namespace {

// TODO(dmauro): Replace with a commandline flag.
static constexpr bool kExtendedTest = false;

std::unique_ptr<abel::synchronization_internal::ThreadPool> CreatePool(
    int threads) {
  return abel::make_unique<abel::synchronization_internal::ThreadPool>(threads);
}

std::unique_ptr<abel::synchronization_internal::ThreadPool>
CreateDefaultPool() {
  return CreatePool(kExtendedTest ? 32 : 10);
}

// Hack to schedule a function to run on a thread pool thread after a
// duration has elapsed.
static void ScheduleAfter(abel::synchronization_internal::ThreadPool *tp,
                          abel::duration after,
                          const std::function<void()> &func) {
  tp->Schedule([func, after] {
    abel::sleep_for(after);
    func();
  });
}

struct TestContext {
  int iterations;
  int threads;
  int g0;  // global 0
  int g1;  // global 1
  abel::mutex mu;
  abel::cond_var cv;
};

// To test whether the invariant check call occurs
static std::atomic<bool> invariant_checked;

static bool GetInvariantChecked() {
  return invariant_checked.load(std::memory_order_relaxed);
}

static void SetInvariantChecked(bool new_value) {
  invariant_checked.store(new_value, std::memory_order_relaxed);
}

static void CheckSumG0G1(void *v) {
  TestContext *cxt = static_cast<TestContext *>(v);
  ABEL_RAW_CHECK(cxt->g0 == -cxt->g1, "Error in CheckSumG0G1");
  SetInvariantChecked(true);
}

static void TestMu(TestContext *cxt, int c) {
  for (int i = 0; i != cxt->iterations; i++) {
    abel::mutex_lock l(&cxt->mu);
    int a = cxt->g0 + 1;
    cxt->g0 = a;
    cxt->g1--;
  }
}

static void TestTry(TestContext *cxt, int c) {
  for (int i = 0; i != cxt->iterations; i++) {
    do {
      std::this_thread::yield();
    } while (!cxt->mu.try_lock());
    int a = cxt->g0 + 1;
    cxt->g0 = a;
    cxt->g1--;
    cxt->mu.unlock();
  }
}

static void TestR20ms(TestContext *cxt, int c) {
  for (int i = 0; i != cxt->iterations; i++) {
    abel::reader_mutex_lock l(&cxt->mu);
    abel::sleep_for(abel::milliseconds(20));
    cxt->mu.assert_reader_held();
  }
}

static void TestRW(TestContext *cxt, int c) {
  if ((c & 1) == 0) {
    for (int i = 0; i != cxt->iterations; i++) {
      abel::writer_mutex_lock l(&cxt->mu);
      cxt->g0++;
      cxt->g1--;
      cxt->mu.assert_held();
      cxt->mu.assert_reader_held();
    }
  } else {
    for (int i = 0; i != cxt->iterations; i++) {
      abel::reader_mutex_lock l(&cxt->mu);
      ABEL_RAW_CHECK(cxt->g0 == -cxt->g1, "Error in TestRW");
      cxt->mu.assert_reader_held();
    }
  }
}

struct MyContext {
  int target;
  TestContext *cxt;
  bool MyTurn();
};

bool MyContext::MyTurn() {
  TestContext *cxt = this->cxt;
  return cxt->g0 == this->target || cxt->g0 == cxt->iterations;
}

static void TestAwait(TestContext *cxt, int c) {
  MyContext mc;
  mc.target = c;
  mc.cxt = cxt;
  abel::mutex_lock l(&cxt->mu);
  cxt->mu.assert_held();
  while (cxt->g0 < cxt->iterations) {
    cxt->mu.Await(abel::condition(&mc, &MyContext::MyTurn));
    ABEL_RAW_CHECK(mc.MyTurn(), "Error in TestAwait");
    cxt->mu.assert_held();
    if (cxt->g0 < cxt->iterations) {
      int a = cxt->g0 + 1;
      cxt->g0 = a;
      mc.target += cxt->threads;
    }
  }
}

static void TestSignalAll(TestContext *cxt, int c) {
  int target = c;
  abel::mutex_lock l(&cxt->mu);
  cxt->mu.assert_held();
  while (cxt->g0 < cxt->iterations) {
    while (cxt->g0 != target && cxt->g0 != cxt->iterations) {
      cxt->cv.wait(&cxt->mu);
    }
    if (cxt->g0 < cxt->iterations) {
      int a = cxt->g0 + 1;
      cxt->g0 = a;
      cxt->cv.signal_all();
      target += cxt->threads;
    }
  }
}

static void TestSignal(TestContext *cxt, int c) {
  ABEL_RAW_CHECK(cxt->threads == 2, "TestSignal should use 2 threads");
  int target = c;
  abel::mutex_lock l(&cxt->mu);
  cxt->mu.assert_held();
  while (cxt->g0 < cxt->iterations) {
    while (cxt->g0 != target && cxt->g0 != cxt->iterations) {
      cxt->cv.wait(&cxt->mu);
    }
    if (cxt->g0 < cxt->iterations) {
      int a = cxt->g0 + 1;
      cxt->g0 = a;
      cxt->cv.signal();
      target += cxt->threads;
    }
  }
}

static void TestCVTimeout(TestContext *cxt, int c) {
  int target = c;
  abel::mutex_lock l(&cxt->mu);
  cxt->mu.assert_held();
  while (cxt->g0 < cxt->iterations) {
    while (cxt->g0 != target && cxt->g0 != cxt->iterations) {
      cxt->cv.wait_with_timeout(&cxt->mu, abel::seconds(100));
    }
    if (cxt->g0 < cxt->iterations) {
      int a = cxt->g0 + 1;
      cxt->g0 = a;
      cxt->cv.signal_all();
      target += cxt->threads;
    }
  }
}

static bool G0GE2(TestContext *cxt) { return cxt->g0 >= 2; }

static void TestTime(TestContext *cxt, int c, bool use_cv) {
  ABEL_RAW_CHECK(cxt->iterations == 1, "TestTime should only use 1 iteration");
  ABEL_RAW_CHECK(cxt->threads > 2, "TestTime should use more than 2 threads");
  const bool kFalse = false;
  abel::condition false_cond(&kFalse);
  abel::condition g0ge2(G0GE2, cxt);
  if (c == 0) {
    abel::mutex_lock l(&cxt->mu);

    abel::abel_time start = abel::now();
    if (use_cv) {
      cxt->cv.wait_with_timeout(&cxt->mu, abel::seconds(1));
    } else {
      ABEL_RAW_CHECK(!cxt->mu.AwaitWithTimeout(false_cond, abel::seconds(1)),
                     "TestTime failed");
    }
    abel::duration elapsed = abel::now() - start;
    ABEL_RAW_CHECK(
        abel::seconds(0.9) <= elapsed && elapsed <= abel::seconds(2.0),
        "TestTime failed");
    ABEL_RAW_CHECK(cxt->g0 == 1, "TestTime failed");

    start = abel::now();
    if (use_cv) {
      cxt->cv.wait_with_timeout(&cxt->mu, abel::seconds(1));
    } else {
      ABEL_RAW_CHECK(!cxt->mu.AwaitWithTimeout(false_cond, abel::seconds(1)),
                     "TestTime failed");
    }
    elapsed = abel::now() - start;
    ABEL_RAW_CHECK(
        abel::seconds(0.9) <= elapsed && elapsed <= abel::seconds(2.0),
        "TestTime failed");
    cxt->g0++;
    if (use_cv) {
      cxt->cv.signal();
    }

    start = abel::now();
    if (use_cv) {
      cxt->cv.wait_with_timeout(&cxt->mu, abel::seconds(4));
    } else {
      ABEL_RAW_CHECK(!cxt->mu.AwaitWithTimeout(false_cond, abel::seconds(4)),
                     "TestTime failed");
    }
    elapsed = abel::now() - start;
    ABEL_RAW_CHECK(
        abel::seconds(3.9) <= elapsed && elapsed <= abel::seconds(6.0),
        "TestTime failed");
    ABEL_RAW_CHECK(cxt->g0 >= 3, "TestTime failed");

    start = abel::now();
    if (use_cv) {
      cxt->cv.wait_with_timeout(&cxt->mu, abel::seconds(1));
    } else {
      ABEL_RAW_CHECK(!cxt->mu.AwaitWithTimeout(false_cond, abel::seconds(1)),
                     "TestTime failed");
    }
    elapsed = abel::now() - start;
    ABEL_RAW_CHECK(
        abel::seconds(0.9) <= elapsed && elapsed <= abel::seconds(2.0),
        "TestTime failed");
    if (use_cv) {
      cxt->cv.signal_all();
    }

    start = abel::now();
    if (use_cv) {
      cxt->cv.wait_with_timeout(&cxt->mu, abel::seconds(1));
    } else {
      ABEL_RAW_CHECK(!cxt->mu.AwaitWithTimeout(false_cond, abel::seconds(1)),
                     "TestTime failed");
    }
    elapsed = abel::now() - start;
    ABEL_RAW_CHECK(abel::seconds(0.9) <= elapsed &&
                   elapsed <= abel::seconds(2.0), "TestTime failed");
    ABEL_RAW_CHECK(cxt->g0 == cxt->threads, "TestTime failed");

  } else if (c == 1) {
    abel::mutex_lock l(&cxt->mu);
    const abel::abel_time start = abel::now();
    if (use_cv) {
      cxt->cv.wait_with_timeout(&cxt->mu, abel::milliseconds(500));
    } else {
      ABEL_RAW_CHECK(
          !cxt->mu.AwaitWithTimeout(false_cond, abel::milliseconds(500)),
          "TestTime failed");
    }
    const abel::duration elapsed = abel::now() - start;
    ABEL_RAW_CHECK(
        abel::seconds(0.4) <= elapsed && elapsed <= abel::seconds(0.9),
        "TestTime failed");
    cxt->g0++;
  } else if (c == 2) {
    abel::mutex_lock l(&cxt->mu);
    if (use_cv) {
      while (cxt->g0 < 2) {
        cxt->cv.wait_with_timeout(&cxt->mu, abel::seconds(100));
      }
    } else {
      ABEL_RAW_CHECK(cxt->mu.AwaitWithTimeout(g0ge2, abel::seconds(100)),
                     "TestTime failed");
    }
    cxt->g0++;
  } else {
    abel::mutex_lock l(&cxt->mu);
    if (use_cv) {
      while (cxt->g0 < 2) {
        cxt->cv.wait(&cxt->mu);
      }
    } else {
      cxt->mu.Await(g0ge2);
    }
    cxt->g0++;
  }
}

static void TestMuTime(TestContext *cxt, int c) { TestTime(cxt, c, false); }

static void TestCVTime(TestContext *cxt, int c) { TestTime(cxt, c, true); }

static void EndTest(int *c0, int *c1, abel::mutex *mu, abel::cond_var *cv,
                    const std::function<void(int)>& cb) {
  mu->lock();
  int c = (*c0)++;
  mu->unlock();
  cb(c);
  abel::mutex_lock l(mu);
  (*c1)++;
  cv->signal();
}

// Code common to RunTest() and RunTestWithInvariantDebugging().
static int RunTestCommon(TestContext *cxt, void (*test)(TestContext *cxt, int),
                         int threads, int iterations, int operations) {
  abel::mutex mu2;
  abel::cond_var cv2;
  int c0 = 0;
  int c1 = 0;
  cxt->g0 = 0;
  cxt->g1 = 0;
  cxt->iterations = iterations;
  cxt->threads = threads;
  abel::synchronization_internal::ThreadPool tp(threads);
  for (int i = 0; i != threads; i++) {
    tp.Schedule(std::bind(&EndTest, &c0, &c1, &mu2, &cv2,
                          std::function<void(int)>(
                              std::bind(test, cxt, std::placeholders::_1))));
  }
  mu2.lock();
  while (c1 != threads) {
    cv2.wait(&mu2);
  }
  mu2.unlock();
  return cxt->g0;
}

// Basis for the parameterized tests configured below.
static int RunTest(void (*test)(TestContext *cxt, int), int threads,
                   int iterations, int operations) {
  TestContext cxt;
  return RunTestCommon(&cxt, test, threads, iterations, operations);
}

// Like RunTest(), but sets an invariant on the tested mutex and
// verifies that the invariant check happened. The invariant function
// will be passed the TestContext* as its arg and must call
// SetInvariantChecked(true);
#if !defined(ABEL_MUTEX_ENABLE_INVARIANT_DEBUGGING_NOT_IMPLEMENTED)
static int RunTestWithInvariantDebugging(void (*test)(TestContext *cxt, int),
                                         int threads, int iterations,
                                         int operations,
                                         void (*invariant)(void *)) {
  abel::enable_mutex_invariant_debugging(true);
  SetInvariantChecked(false);
  TestContext cxt;
  cxt.mu.EnableInvariantDebugging(invariant, &cxt);
  int ret = RunTestCommon(&cxt, test, threads, iterations, operations);
  ABEL_RAW_CHECK(GetInvariantChecked(), "Invariant not checked");
  abel::enable_mutex_invariant_debugging(false);  // Restore.
  return ret;
}
#endif

// --------------------------------------------------------
// Test for fix of bug in TryRemove()
struct TimeoutBugStruct {
  abel::mutex mu;
  bool a;
  int a_waiter_count;
};

static void WaitForA(TimeoutBugStruct *x) {
  x->mu.LockWhen(abel::condition(&x->a));
  x->a_waiter_count--;
  x->mu.unlock();
}

static bool NoAWaiters(TimeoutBugStruct *x) { return x->a_waiter_count == 0; }

// Test that a cond_var.wait(&mutex) can un-block a call to mutex.Await() in
// another thread.
TEST(mutex, CondVarWaitSignalsAwait) {
  // Use a struct so the lock annotations apply.
  struct {
    abel::mutex barrier_mu;
    bool barrier ABEL_GUARDED_BY(barrier_mu) = false;

    abel::mutex release_mu;
    bool release ABEL_GUARDED_BY(release_mu) = false;
    abel::cond_var released_cv;
  } state;

  auto pool = CreateDefaultPool();

  // Thread A.  Sets barrier, waits for release using mutex::Await, then
  // signals released_cv.
  pool->Schedule([&state] {
    state.release_mu.lock();

    state.barrier_mu.lock();
    state.barrier = true;
    state.barrier_mu.unlock();

    state.release_mu.Await(abel::condition(&state.release));
    state.released_cv.signal();
    state.release_mu.unlock();
  });

  state.barrier_mu.LockWhen(abel::condition(&state.barrier));
  state.barrier_mu.unlock();
  state.release_mu.lock();
  // Thread A is now blocked on release by way of mutex::Await().

  // Set release.  Calling released_cv.wait() should un-block thread A,
  // which will signal released_cv.  If not, the test will hang.
  state.release = true;
  state.released_cv.wait(&state.release_mu);
  state.release_mu.unlock();
}

// Test that a cond_var.wait_with_timeout(&mutex) can un-block a call to
// mutex.Await() in another thread.
TEST(mutex, CondVarWaitWithTimeoutSignalsAwait) {
  // Use a struct so the lock annotations apply.
  struct {
    abel::mutex barrier_mu;
    bool barrier ABEL_GUARDED_BY(barrier_mu) = false;

    abel::mutex release_mu;
    bool release ABEL_GUARDED_BY(release_mu) = false;
    abel::cond_var released_cv;
  } state;

  auto pool = CreateDefaultPool();

  // Thread A.  Sets barrier, waits for release using mutex::Await, then
  // signals released_cv.
  pool->Schedule([&state] {
    state.release_mu.lock();

    state.barrier_mu.lock();
    state.barrier = true;
    state.barrier_mu.unlock();

    state.release_mu.Await(abel::condition(&state.release));
    state.released_cv.signal();
    state.release_mu.unlock();
  });

  state.barrier_mu.LockWhen(abel::condition(&state.barrier));
  state.barrier_mu.unlock();
  state.release_mu.lock();
  // Thread A is now blocked on release by way of mutex::Await().

  // Set release.  Calling released_cv.wait() should un-block thread A,
  // which will signal released_cv.  If not, the test will hang.
  state.release = true;
  EXPECT_TRUE(
      !state.released_cv.wait_with_timeout(&state.release_mu, abel::seconds(10)))
      << "; Unrecoverable test failure: cond_var::wait_with_timeout did not "
         "unblock the abel::mutex::Await call in another thread.";

  state.release_mu.unlock();
}

// Test for regression of a bug in loop of TryRemove()
TEST(mutex, MutexTimeoutBug) {
  auto tp = CreateDefaultPool();

  TimeoutBugStruct x;
  x.a = false;
  x.a_waiter_count = 2;
  tp->Schedule(std::bind(&WaitForA, &x));
  tp->Schedule(std::bind(&WaitForA, &x));
  abel::sleep_for(abel::seconds(1));  // Allow first two threads to hang.
  // The skip field of the second will point to the first because there are
  // only two.

  // Now cause a thread waiting on an always-false to time out
  // This would deadlock when the bug was present.
  bool always_false = false;
  x.mu.LockWhenWithTimeout(abel::condition(&always_false),
                           abel::milliseconds(500));

  // if we get here, the bug is not present.   Cleanup the state.

  x.a = true;                                    // wakeup the two waiters on A
  x.mu.Await(abel::condition(&NoAWaiters, &x));  // wait for them to exit
  x.mu.unlock();
}

struct CondVarWaitDeadlock : testing::TestWithParam<int> {
  abel::mutex mu;
  abel::cond_var cv;
  bool cond1 = false;
  bool cond2 = false;
  bool read_lock1;
  bool read_lock2;
  bool signal_unlocked;

  CondVarWaitDeadlock() {
    read_lock1 = GetParam() & (1 << 0);
    read_lock2 = GetParam() & (1 << 1);
    signal_unlocked = GetParam() & (1 << 2);
  }

  void Waiter1() {
    if (read_lock1) {
      mu.reader_lock();
      while (!cond1) {
        cv.wait(&mu);
      }
      mu.reader_unlock();
    } else {
      mu.lock();
      while (!cond1) {
        cv.wait(&mu);
      }
      mu.unlock();
    }
  }

  void Waiter2() {
    if (read_lock2) {
      mu.ReaderLockWhen(abel::condition(&cond2));
      mu.reader_unlock();
    } else {
      mu.LockWhen(abel::condition(&cond2));
      mu.unlock();
    }
  }
};

// Test for a deadlock bug in mutex::Fer().
// The sequence of events that lead to the deadlock is:
// 1. waiter1 blocks on cv in read mode (mu bits = 0).
// 2. waiter2 blocks on mu in either mode (mu bits = kMuWait).
// 3. main thread locks mu, sets cond1, unlocks mu (mu bits = kMuWait).
// 4. main thread signals on cv and this eventually calls mutex::Fer().
// Currently Fer wakes waiter1 since mu bits = kMuWait (mutex is unlocked).
// Before the bug fix Fer neither woke waiter1 nor queued it on mutex,
// which resulted in deadlock.
TEST_P(CondVarWaitDeadlock, Test) {
  auto waiter1 = CreatePool(1);
  auto waiter2 = CreatePool(1);
  waiter1->Schedule([this] { this->Waiter1(); });
  waiter2->Schedule([this] { this->Waiter2(); });

  // wait while threads block (best-effort is fine).
  abel::sleep_for(abel::milliseconds(100));

  // Wake condwaiter.
  mu.lock();
  cond1 = true;
  if (signal_unlocked) {
    mu.unlock();
    cv.signal();
  } else {
    cv.signal();
    mu.unlock();
  }
  waiter1.reset();  // "join" waiter1

  // Wake waiter.
  mu.lock();
  cond2 = true;
  mu.unlock();
  waiter2.reset();  // "join" waiter2
}

INSTANTIATE_TEST_SUITE_P(CondVarWaitDeadlockTest, CondVarWaitDeadlock,
                         ::testing::Range(0, 8),
                         ::testing::PrintToStringParamName());

// --------------------------------------------------------
// Test for fix of bug in DequeueAllWakeable()
// Bug was that if there was more than one waiting reader
// and all should be woken, the most recently blocked one
// would not be.

struct DequeueAllWakeableBugStruct {
  abel::mutex mu;
  abel::mutex mu2;       // protects all fields below
  int unfinished_count;  // count of unfinished readers; under mu2
  bool done1;            // unfinished_count == 0; under mu2
  int finished_count;    // count of finished readers, under mu2
  bool done2;            // finished_count == 0; under mu2
};

// Test for regression of a bug in loop of DequeueAllWakeable()
static void AcquireAsReader(DequeueAllWakeableBugStruct *x) {
  x->mu.reader_lock();
  x->mu2.lock();
  x->unfinished_count--;
  x->done1 = (x->unfinished_count == 0);
  x->mu2.unlock();
  // make sure that both readers acquired mu before we release it.
  abel::sleep_for(abel::seconds(2));
  x->mu.reader_unlock();

  x->mu2.lock();
  x->finished_count--;
  x->done2 = (x->finished_count == 0);
  x->mu2.unlock();
}

// Test for regression of a bug in loop of DequeueAllWakeable()
TEST(mutex, MutexReaderWakeupBug) {
  auto tp = CreateDefaultPool();

  DequeueAllWakeableBugStruct x;
  x.unfinished_count = 2;
  x.done1 = false;
  x.finished_count = 2;
  x.done2 = false;
  x.mu.lock();  // acquire mu exclusively
  // queue two thread that will block on reader locks on x.mu
  tp->Schedule(std::bind(&AcquireAsReader, &x));
  tp->Schedule(std::bind(&AcquireAsReader, &x));
  abel::sleep_for(abel::seconds(1));  // give time for reader threads to block
  x.mu.unlock();                     // wake them up

  // both readers should finish promptly
  EXPECT_TRUE(
      x.mu2.LockWhenWithTimeout(abel::condition(&x.done1), abel::seconds(10)));
  x.mu2.unlock();

  EXPECT_TRUE(
      x.mu2.LockWhenWithTimeout(abel::condition(&x.done2), abel::seconds(10)));
  x.mu2.unlock();
}

struct LockWhenTestStruct {
  abel::mutex mu1;
  bool cond = false;

  abel::mutex mu2;
  bool waiting = false;
};

static bool LockWhenTestIsCond(LockWhenTestStruct* s) {
  s->mu2.lock();
  s->waiting = true;
  s->mu2.unlock();
  return s->cond;
}

static void LockWhenTestWaitForIsCond(LockWhenTestStruct* s) {
  s->mu1.LockWhen(abel::condition(&LockWhenTestIsCond, s));
  s->mu1.unlock();
}

TEST(mutex, LockWhen) {
  LockWhenTestStruct s;

  std::thread t(LockWhenTestWaitForIsCond, &s);
  s.mu2.LockWhen(abel::condition(&s.waiting));
  s.mu2.unlock();

  s.mu1.lock();
  s.cond = true;
  s.mu1.unlock();

  t.join();
}

// --------------------------------------------------------
// The following test requires mutex::reader_lock to be a real shared
// lock, which is not the case in all builds.
#if !defined(ABEL_MUTEX_READER_LOCK_IS_EXCLUSIVE)

// Test for fix of bug in UnlockSlow() that incorrectly decremented the reader
// count when putting a thread to sleep waiting for a false condition when the
// lock was not held.

// For this bug to strike, we make a thread wait on a free mutex with no
// waiters by causing its wakeup condition to be false.   Then the
// next two acquirers must be readers.   The bug causes the lock
// to be released when one reader unlocks, rather than both.

struct ReaderDecrementBugStruct {
  bool cond;  // to delay first thread (under mu)
  int done;   // reference count (under mu)
  abel::mutex mu;

  bool waiting_on_cond;   // under mu2
  bool have_reader_lock;  // under mu2
  bool complete;          // under mu2
  abel::mutex mu2;        // > mu
};

// L >= mu, L < mu_waiting_on_cond
static bool IsCond(void *v) {
  ReaderDecrementBugStruct *x = reinterpret_cast<ReaderDecrementBugStruct *>(v);
  x->mu2.lock();
  x->waiting_on_cond = true;
  x->mu2.unlock();
  return x->cond;
}

// L >= mu
static bool AllDone(void *v) {
  ReaderDecrementBugStruct *x = reinterpret_cast<ReaderDecrementBugStruct *>(v);
  return x->done == 0;
}

// L={}
static void WaitForCond(ReaderDecrementBugStruct *x) {
  abel::mutex dummy;
  abel::mutex_lock l(&dummy);
  x->mu.LockWhen(abel::condition(&IsCond, x));
  x->done--;
  x->mu.unlock();
}

// L={}
static void GetReadLock(ReaderDecrementBugStruct *x) {
  x->mu.reader_lock();
  x->mu2.lock();
  x->have_reader_lock = true;
  x->mu2.Await(abel::condition(&x->complete));
  x->mu2.unlock();
  x->mu.reader_unlock();
  x->mu.lock();
  x->done--;
  x->mu.unlock();
}

// Test for reader counter being decremented incorrectly by waiter
// with false condition.
TEST(mutex, MutexReaderDecrementBug) ABEL_NO_THREAD_SAFETY_ANALYSIS {
  ReaderDecrementBugStruct x;
  x.cond = false;
  x.waiting_on_cond = false;
  x.have_reader_lock = false;
  x.complete = false;
  x.done = 2;  // initial ref count

  // Run WaitForCond() and wait for it to sleep
  std::thread thread1(WaitForCond, &x);
  x.mu2.LockWhen(abel::condition(&x.waiting_on_cond));
  x.mu2.unlock();

  // Run GetReadLock(), and wait for it to get the read lock
  std::thread thread2(GetReadLock, &x);
  x.mu2.LockWhen(abel::condition(&x.have_reader_lock));
  x.mu2.unlock();

  // Get the reader lock ourselves, and release it.
  x.mu.reader_lock();
  x.mu.reader_unlock();

  // The lock should be held in read mode by GetReadLock().
  // If we have the bug, the lock will be free.
  x.mu.assert_reader_held();

  // Wake up all the threads.
  x.mu2.lock();
  x.complete = true;
  x.mu2.unlock();

  // TODO(delesley): turn on analysis once lock upgrading is supported.
  // (This call upgrades the lock from shared to exclusive.)
  x.mu.lock();
  x.cond = true;
  x.mu.Await(abel::condition(&AllDone, &x));
  x.mu.unlock();

  thread1.join();
  thread2.join();
}
#endif  // !ABEL_MUTEX_READER_LOCK_IS_EXCLUSIVE

// Test that we correctly handle the situation when a lock is
// held and then destroyed (w/o unlocking).
#ifdef THREAD_SANITIZER
// TSAN reports errors when locked Mutexes are destroyed.
TEST(mutex, DISABLED_LockedMutexDestructionBug) NO_THREAD_SAFETY_ANALYSIS {
#else
TEST(mutex, LockedMutexDestructionBug) ABEL_NO_THREAD_SAFETY_ANALYSIS {
#endif
  for (int i = 0; i != 10; i++) {
    // Create, lock and destroy 10 locks.
    const int kNumLocks = 10;
    auto mu = abel::make_unique<abel::mutex[]>(kNumLocks);
    for (int j = 0; j != kNumLocks; j++) {
      if ((j % 2) == 0) {
        mu[j].writer_lock();
      } else {
        mu[j].reader_lock();
      }
    }
  }
}

// --------------------------------------------------------
// Test for bug with pattern of readers using a condvar.  The bug was that if a
// reader went to sleep on a condition variable while one or more other readers
// held the lock, but there were no waiters, the reader count (held in the
// mutex word) would be lost.  (This is because Enqueue() had at one time
// always placed the thread on the mutex queue.  Later (CL 4075610), to
// tolerate re-entry into mutex from a condition predicate, Enqueue() was
// changed so that it could also place a thread on a condition-variable.  This
// introduced the case where Enqueue() returned with an empty queue, and this
// case was handled incorrectly in one place.)

static void ReaderForReaderOnCondVar(abel::mutex *mu, abel::cond_var *cv,
                                     int *running) {
  std::random_device dev;
  std::mt19937 gen(dev());
  std::uniform_int_distribution<int> random_millis(0, 15);
  mu->reader_lock();
  while (*running == 3) {
    abel::sleep_for(abel::milliseconds(random_millis(gen)));
    cv->wait_with_timeout(mu, abel::milliseconds(random_millis(gen)));
  }
  mu->reader_unlock();
  mu->lock();
  (*running)--;
  mu->unlock();
}

struct True {
  template <class... Args>
  bool operator()(Args...) const {
    return true;
  }
};

struct DerivedTrue : True {};

TEST(mutex, FunctorCondition) {
  {  // Variadic
    True f;
    EXPECT_TRUE(abel::condition(&f).eval());
  }

  {  // Inherited
    DerivedTrue g;
    EXPECT_TRUE(abel::condition(&g).eval());
  }

  {  // lambda
    int value = 3;
    auto is_zero = [&value] { return value == 0; };
    abel::condition c(&is_zero);
    EXPECT_FALSE(c.eval());
    value = 0;
    EXPECT_TRUE(c.eval());
  }

  {  // bind
    int value = 0;
    auto is_positive = std::bind(std::less<int>(), 0, std::cref(value));
    abel::condition c(&is_positive);
    EXPECT_FALSE(c.eval());
    value = 1;
    EXPECT_TRUE(c.eval());
  }

  {  // std::function
    int value = 3;
    std::function<bool()> is_zero = [&value] { return value == 0; };
    abel::condition c(&is_zero);
    EXPECT_FALSE(c.eval());
    value = 0;
    EXPECT_TRUE(c.eval());
  }
}

static bool IntIsZero(int *x) { return *x == 0; }

// Test for reader waiting condition variable when there are other readers
// but no waiters.
TEST(mutex, TestReaderOnCondVar) {
  auto tp = CreateDefaultPool();
  abel::mutex mu;
  abel::cond_var cv;
  int running = 3;
  tp->Schedule(std::bind(&ReaderForReaderOnCondVar, &mu, &cv, &running));
  tp->Schedule(std::bind(&ReaderForReaderOnCondVar, &mu, &cv, &running));
  abel::sleep_for(abel::seconds(2));
  mu.lock();
  running--;
  mu.Await(abel::condition(&IntIsZero, &running));
  mu.unlock();
}

// --------------------------------------------------------
struct AcquireFromConditionStruct {
  abel::mutex mu0;   // protects value, done
  int value;         // times condition function is called; under mu0,
  bool done;         // done with test?  under mu0
  abel::mutex mu1;   // used to attempt to mess up state of mu0
  abel::cond_var cv;  // so the condition function can be invoked from
                     // cond_var::wait().
};

static bool ConditionWithAcquire(AcquireFromConditionStruct *x) {
  x->value++;  // count times this function is called

  if (x->value == 2 || x->value == 3) {
    // On the second and third invocation of this function, sleep for 100ms,
    // but with the side-effect of altering the state of a mutex other than
    // than one for which this is a condition.  The spec now explicitly allows
    // this side effect; previously it did not.  it was illegal.
    bool always_false = false;
    x->mu1.LockWhenWithTimeout(abel::condition(&always_false),
                               abel::milliseconds(100));
    x->mu1.unlock();
  }
  ABEL_RAW_CHECK(x->value < 4, "should not be invoked a fourth time");

  // We arrange for the condition to return true on only the 2nd and 3rd calls.
  return x->value == 2 || x->value == 3;
}

static void WaitForCond2(AcquireFromConditionStruct *x) {
  // wait for cond0 to become true
  x->mu0.LockWhen(abel::condition(&ConditionWithAcquire, x));
  x->done = true;
  x->mu0.unlock();
}

// Test for condition whose function acquires other Mutexes
TEST(mutex, AcquireFromCondition) {
  auto tp = CreateDefaultPool();

  AcquireFromConditionStruct x;
  x.value = 0;
  x.done = false;
  tp->Schedule(
      std::bind(&WaitForCond2, &x));  // run WaitForCond2() in a thread T
  // T will hang because the first invocation of ConditionWithAcquire() will
  // return false.
  abel::sleep_for(abel::milliseconds(500));  // allow T time to hang

  x.mu0.lock();
  x.cv.wait_with_timeout(&x.mu0, abel::milliseconds(500));  // wake T
  // T will be woken because the wait() will call ConditionWithAcquire()
  // for the second time, and it will return true.

  x.mu0.unlock();

  // T will then acquire the lock and recheck its own condition.
  // It will find the condition true, as this is the third invocation,
  // but the use of another mutex by the calling function will
  // cause the old mutex implementation to think that the outer
  // LockWhen() has timed out because the inner LockWhenWithTimeout() did.
  // T will then check the condition a fourth time because it finds a
  // timeout occurred.  This should not happen in the new
  // implementation that allows the condition function to use Mutexes.

  // It should also succeed, even though the condition function
  // is being invoked from cond_var::wait, and thus this thread
  // is conceptually waiting both on the condition variable, and on mu2.

  x.mu0.LockWhen(abel::condition(&x.done));
  x.mu0.unlock();
}

// The deadlock detector is not part of non-prod builds, so do not test it.
#if !defined(ABEL_INTERNAL_USE_NONPROD_MUTEX)

TEST(mutex, DeadlockDetector) {
  abel::set_mutex_deadlock_detection_mode(abel::on_deadlock_cycle::kAbort);

  // check that we can call ForgetDeadlockInfo() on a lock with the lock held
  abel::mutex m1;
  abel::mutex m2;
  abel::mutex m3;
  abel::mutex m4;

  m1.lock();  // m1 gets ID1
  m2.lock();  // m2 gets ID2
  m3.lock();  // m3 gets ID3
  m3.unlock();
  m2.unlock();
  // m1 still held
  m1.ForgetDeadlockInfo();  // m1 loses ID
  m2.lock();                // m2 gets ID2
  m3.lock();                // m3 gets ID3
  m4.lock();                // m4 gets ID4
  m3.unlock();
  m2.unlock();
  m4.unlock();
  m1.unlock();
}

// Bazel has a test "warning" file that programs can write to if the
// test should pass with a warning.  This class disables the warning
// file until it goes out of scope.
class ScopedDisableBazelTestWarnings {
 public:
  ScopedDisableBazelTestWarnings() {
#ifdef _WIN32
    char file[MAX_PATH];
    if (GetEnvironmentVariableA(kVarName, file, sizeof(file)) < sizeof(file)) {
      warnings_output_file_ = file;
      SetEnvironmentVariableA(kVarName, nullptr);
    }
#else
    const char *file = getenv(kVarName);
    if (file != nullptr) {
      warnings_output_file_ = file;
      unsetenv(kVarName);
    }
#endif
  }

  ~ScopedDisableBazelTestWarnings() {
    if (!warnings_output_file_.empty()) {
#ifdef _WIN32
      SetEnvironmentVariableA(kVarName, warnings_output_file_.c_str());
#else
      setenv(kVarName, warnings_output_file_.c_str(), 0);
#endif
    }
  }

 private:
  static const char kVarName[];
  std::string warnings_output_file_;
};
const char ScopedDisableBazelTestWarnings::kVarName[] =
    "TEST_WARNINGS_OUTPUT_FILE";

#ifdef THREAD_SANITIZER
// This test intentionally creates deadlocks to test the deadlock detector.
TEST(mutex, DISABLED_DeadlockDetectorBazelWarning) {
#else
TEST(mutex, DeadlockDetectorBazelWarning) {
#endif
  abel::set_mutex_deadlock_detection_mode(abel::on_deadlock_cycle::kReport);

  // Cause deadlock detection to detect something, if it's
  // compiled in and enabled.  But turn off the bazel warning.
  ScopedDisableBazelTestWarnings disable_bazel_test_warnings;

  abel::mutex mu0;
  abel::mutex mu1;
  bool got_mu0 = mu0.try_lock();
  mu1.lock();  // acquire mu1 while holding mu0
  if (got_mu0) {
    mu0.unlock();
  }
  if (mu0.try_lock()) {  // try lock shouldn't cause deadlock detector to fire
    mu0.unlock();
  }
  mu0.lock();  // acquire mu0 while holding mu1; should get one deadlock
               // report here
  mu0.unlock();
  mu1.unlock();

  abel::set_mutex_deadlock_detection_mode(abel::on_deadlock_cycle::kAbort);
}

// This test is tagged with NO_THREAD_SAFETY_ANALYSIS because the
// annotation-based static thread-safety analysis is not currently
// predicate-aware and cannot tell if the two for-loops that acquire and
// release the locks have the same predicates.
TEST(mutex, DeadlockDetectorStessTest) ABEL_NO_THREAD_SAFETY_ANALYSIS {
  // Stress test: Here we create a large number of locks and use all of them.
  // If a deadlock detector keeps a full graph of lock acquisition order,
  // it will likely be too slow for this test to pass.
  const int n_locks = 1 << 17;
  auto array_of_locks = abel::make_unique<abel::mutex[]>(n_locks);
  for (int i = 0; i < n_locks; i++) {
    int end = std::min(n_locks, i + 5);
    // acquire and then release locks i, i+1, ..., i+4
    for (int j = i; j < end; j++) {
      array_of_locks[j].lock();
    }
    for (int j = i; j < end; j++) {
      array_of_locks[j].unlock();
    }
  }
}

#ifdef THREAD_SANITIZER
// TSAN reports errors when locked Mutexes are destroyed.
TEST(mutex, DISABLED_DeadlockIdBug) NO_THREAD_SAFETY_ANALYSIS {
#else
TEST(mutex, DeadlockIdBug) ABEL_NO_THREAD_SAFETY_ANALYSIS {
#endif
  // Test a scenario where a cached deadlock graph node id in the
  // list of held locks is not invalidated when the corresponding
  // mutex is deleted.
  abel::set_mutex_deadlock_detection_mode(abel::on_deadlock_cycle::kAbort);
  // mutex that will be destroyed while being held
  abel::mutex *a = new abel::mutex;
  // Other mutexes needed by test
  abel::mutex b, c;

  // Hold mutex.
  a->lock();

  // Force deadlock id assignment by acquiring another lock.
  b.lock();
  b.unlock();

  // Delete the mutex. The mutex destructor tries to remove held locks,
  // but the attempt isn't foolproof.  It can fail if:
  //   (a) Deadlock detection is currently disabled.
  //   (b) The destruction is from another thread.
  // We exploit (a) by temporarily disabling deadlock detection.
  abel::set_mutex_deadlock_detection_mode(abel::on_deadlock_cycle::kIgnore);
  delete a;
  abel::set_mutex_deadlock_detection_mode(abel::on_deadlock_cycle::kAbort);

  // Now acquire another lock which will force a deadlock id assignment.
  // We should end up getting assigned the same deadlock id that was
  // freed up when "a" was deleted, which will cause a spurious deadlock
  // report if the held lock entry for "a" was not invalidated.
  c.lock();
  c.unlock();
}
#endif  // !defined(ABEL_INTERNAL_USE_NONPROD_MUTEX)

// --------------------------------------------------------
// Test for timeouts/deadlines on condition waits that are specified using
// abel::duration and abel::abel_time.  For each waiting function we test with
// a timeout/deadline that has already expired/passed, one that is infinite
// and so never expires/passes, and one that will expire/pass in the near
// future.

static abel::duration TimeoutTestAllowedSchedulingDelay() {
  // Note: we use a function here because Microsoft Visual Studio fails to
  // properly initialize constexpr static abel::duration variables.
  return abel::milliseconds(150);
}

// Returns true if `actual_delay` is close enough to `expected_delay` to pass
// the timeouts/deadlines test.  Otherwise, logs warnings and returns false.
ABEL_MUST_USE_RESULT
static bool DelayIsWithinBounds(abel::duration expected_delay,
                                abel::duration actual_delay) {
  bool pass = true;
  // Do not allow the observed delay to be less than expected.  This may occur
  // in practice due to clock skew or when the synchronization primitives use a
  // different clock than abel::now(), but these cases should be handled by the
  // the retry mechanism in each TimeoutTest.
  if (actual_delay < expected_delay) {
    ABEL_RAW_LOG(WARNING,
                 "Actual delay %s was too short, expected %s (difference %s)",
                 abel::format_duration(actual_delay).c_str(),
                 abel::format_duration(expected_delay).c_str(),
                 abel::format_duration(actual_delay - expected_delay).c_str());
    pass = false;
  }
  // If the expected delay is <= zero then allow a small error tolerance, since
  // we do not expect context switches to occur during test execution.
  // Otherwise, thread scheduling delays may be substantial in rare cases, so
  // tolerate up to kTimeoutTestAllowedSchedulingDelay of error.
  abel::duration tolerance = expected_delay <= abel::zero_duration()
                                 ? abel::milliseconds(10)
                                 : TimeoutTestAllowedSchedulingDelay();
  if (actual_delay > expected_delay + tolerance) {
    ABEL_RAW_LOG(WARNING,
                 "Actual delay %s was too long, expected %s (difference %s)",
                 abel::format_duration(actual_delay).c_str(),
                 abel::format_duration(expected_delay).c_str(),
                 abel::format_duration(actual_delay - expected_delay).c_str());
    pass = false;
  }
  return pass;
}

// Parameters for TimeoutTest, below.
struct TimeoutTestParam {
  // The file and line number (used for logging purposes only).
  const char *from_file;
  int from_line;

  // Should the absolute deadline API based on abel::abel_time be tested?  If false,
  // the relative deadline API based on abel::duration is tested.
  bool use_absolute_deadline;

  // The deadline/timeout used when calling the API being tested
  // (e.g. mutex::LockWhenWithDeadline).
  abel::duration wait_timeout;

  // The delay before the condition will be set true by the test code.  If zero
  // or negative, the condition is set true immediately (before calling the API
  // being tested).  Otherwise, if infinite, the condition is never set true.
  // Otherwise a closure is scheduled for the future that sets the condition
  // true.
  abel::duration satisfy_condition_delay;

  // The expected result of the condition after the call to the API being
  // tested. Generally `true` means the condition was true when the API returns,
  // `false` indicates an expected timeout.
  bool expected_result;

  // The expected delay before the API under test returns.  This is inherently
  // flaky, so some slop is allowed (see `DelayIsWithinBounds` above), and the
  // test keeps trying indefinitely until this constraint passes.
  abel::duration expected_delay;
};

// Print a `TimeoutTestParam` to a debug log.
std::ostream &operator<<(std::ostream &os, const TimeoutTestParam &param) {
  return os << "from: " << param.from_file << ":" << param.from_line
            << " use_absolute_deadline: "
            << (param.use_absolute_deadline ? "true" : "false")
            << " wait_timeout: " << param.wait_timeout
            << " satisfy_condition_delay: " << param.satisfy_condition_delay
            << " expected_result: "
            << (param.expected_result ? "true" : "false")
            << " expected_delay: " << param.expected_delay;
}

std::string FormatString(const TimeoutTestParam &param) {
  std::ostringstream os;
  os << param;
  return os.str();
}

// Like `thread::Executor::ScheduleAt` except:
// a) Delays zero or negative are executed immediately in the current thread.
// b) Infinite delays are never scheduled.
// c) Calls this test's `ScheduleAt` helper instead of using `pool` directly.
static void RunAfterDelay(abel::duration delay,
                          abel::synchronization_internal::ThreadPool *pool,
                          const std::function<void()> &callback) {
  if (delay <= abel::zero_duration()) {
    callback();  // immediate
  } else if (delay != abel::infinite_duration()) {
    ScheduleAfter(pool, delay, callback);
  }
}

class TimeoutTest : public ::testing::Test,
                    public ::testing::WithParamInterface<TimeoutTestParam> {};

std::vector<TimeoutTestParam> MakeTimeoutTestParamValues() {
  // The `finite` delay is a finite, relatively short, delay.  We make it larger
  // than our allowed scheduling delay (slop factor) to avoid confusion when
  // diagnosing test failures.  The other constants here have clear meanings.
  const abel::duration finite = 3 * TimeoutTestAllowedSchedulingDelay();
  const abel::duration never = abel::infinite_duration();
  const abel::duration negative = -abel::infinite_duration();
  const abel::duration immediate = abel::zero_duration();

  // Every test case is run twice; once using the absolute deadline API and once
  // using the relative timeout API.
  std::vector<TimeoutTestParam> values;
  for (bool use_absolute_deadline : {false, true}) {
    // Tests with a negative timeout (deadline in the past), which should
    // immediately return current state of the condition.

    // The condition is already true:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        negative,   // wait_timeout
        immediate,  // satisfy_condition_delay
        true,       // expected_result
        immediate,  // expected_delay
    });

    // The condition becomes true, but the timeout has already expired:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        negative,  // wait_timeout
        finite,    // satisfy_condition_delay
        false,     // expected_result
        immediate  // expected_delay
    });

    // The condition never becomes true:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        negative,  // wait_timeout
        never,     // satisfy_condition_delay
        false,     // expected_result
        immediate  // expected_delay
    });

    // Tests with an infinite timeout (deadline in the infinite future), which
    // should only return when the condition becomes true.

    // The condition is already true:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        never,      // wait_timeout
        immediate,  // satisfy_condition_delay
        true,       // expected_result
        immediate   // expected_delay
    });

    // The condition becomes true before the (infinite) expiry:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        never,   // wait_timeout
        finite,  // satisfy_condition_delay
        true,    // expected_result
        finite,  // expected_delay
    });

    // Tests with a (small) finite timeout (deadline soon), with the condition
    // becoming true both before and after its expiry.

    // The condition is already true:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        never,      // wait_timeout
        immediate,  // satisfy_condition_delay
        true,       // expected_result
        immediate   // expected_delay
    });

    // The condition becomes true before the expiry:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        finite * 2,  // wait_timeout
        finite,      // satisfy_condition_delay
        true,        // expected_result
        finite       // expected_delay
    });

    // The condition becomes true, but the timeout has already expired:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        finite,      // wait_timeout
        finite * 2,  // satisfy_condition_delay
        false,       // expected_result
        finite       // expected_delay
    });

    // The condition never becomes true:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        finite,  // wait_timeout
        never,   // satisfy_condition_delay
        false,   // expected_result
        finite   // expected_delay
    });
  }
  return values;
}

// Instantiate `TimeoutTest` with `MakeTimeoutTestParamValues()`.
INSTANTIATE_TEST_SUITE_P(All, TimeoutTest,
                         testing::ValuesIn(MakeTimeoutTestParamValues()));

TEST_P(TimeoutTest, Await) {
  const TimeoutTestParam params = GetParam();
  ABEL_RAW_LOG(INFO, "Params: %s", FormatString(params).c_str());

  // Because this test asserts bounds on scheduling delays it is flaky.  To
  // compensate it loops forever until it passes.  Failures express as test
  // timeouts, in which case the test log can be used to diagnose the issue.
  for (int attempt = 1;; ++attempt) {
    ABEL_RAW_LOG(INFO, "Attempt %d", attempt);

    abel::mutex mu;
    bool value = false;  // condition value (under mu)

    std::unique_ptr<abel::synchronization_internal::ThreadPool> pool =
        CreateDefaultPool();
    RunAfterDelay(params.satisfy_condition_delay, pool.get(), [&] {
      abel::mutex_lock l(&mu);
      value = true;
    });

    abel::mutex_lock lock(&mu);
    abel::abel_time start_time = abel::now();
    abel::condition cond(&value);
    bool result =
        params.use_absolute_deadline
            ? mu.AwaitWithDeadline(cond, start_time + params.wait_timeout)
            : mu.AwaitWithTimeout(cond, params.wait_timeout);
    if (DelayIsWithinBounds(params.expected_delay, abel::now() - start_time)) {
      EXPECT_EQ(params.expected_result, result);
      break;
    }
  }
}

TEST_P(TimeoutTest, LockWhen) {
  const TimeoutTestParam params = GetParam();
  ABEL_RAW_LOG(INFO, "Params: %s", FormatString(params).c_str());

  // Because this test asserts bounds on scheduling delays it is flaky.  To
  // compensate it loops forever until it passes.  Failures express as test
  // timeouts, in which case the test log can be used to diagnose the issue.
  for (int attempt = 1;; ++attempt) {
    ABEL_RAW_LOG(INFO, "Attempt %d", attempt);

    abel::mutex mu;
    bool value = false;  // condition value (under mu)

    std::unique_ptr<abel::synchronization_internal::ThreadPool> pool =
        CreateDefaultPool();
    RunAfterDelay(params.satisfy_condition_delay, pool.get(), [&] {
      abel::mutex_lock l(&mu);
      value = true;
    });

    abel::abel_time start_time = abel::now();
    abel::condition cond(&value);
    bool result =
        params.use_absolute_deadline
            ? mu.LockWhenWithDeadline(cond, start_time + params.wait_timeout)
            : mu.LockWhenWithTimeout(cond, params.wait_timeout);
    mu.unlock();

    if (DelayIsWithinBounds(params.expected_delay, abel::now() - start_time)) {
      EXPECT_EQ(params.expected_result, result);
      break;
    }
  }
}

TEST_P(TimeoutTest, ReaderLockWhen) {
  const TimeoutTestParam params = GetParam();
  ABEL_RAW_LOG(INFO, "Params: %s", FormatString(params).c_str());

  // Because this test asserts bounds on scheduling delays it is flaky.  To
  // compensate it loops forever until it passes.  Failures express as test
  // timeouts, in which case the test log can be used to diagnose the issue.
  for (int attempt = 0;; ++attempt) {
    ABEL_RAW_LOG(INFO, "Attempt %d", attempt);

    abel::mutex mu;
    bool value = false;  // condition value (under mu)

    std::unique_ptr<abel::synchronization_internal::ThreadPool> pool =
        CreateDefaultPool();
    RunAfterDelay(params.satisfy_condition_delay, pool.get(), [&] {
      abel::mutex_lock l(&mu);
      value = true;
    });

    abel::abel_time start_time = abel::now();
    bool result =
        params.use_absolute_deadline
            ? mu.ReaderLockWhenWithDeadline(abel::condition(&value),
                                            start_time + params.wait_timeout)
            : mu.ReaderLockWhenWithTimeout(abel::condition(&value),
                                           params.wait_timeout);
    mu.reader_unlock();

    if (DelayIsWithinBounds(params.expected_delay, abel::now() - start_time)) {
      EXPECT_EQ(params.expected_result, result);
      break;
    }
  }
}

TEST_P(TimeoutTest, wait) {
  const TimeoutTestParam params = GetParam();
  ABEL_RAW_LOG(INFO, "Params: %s", FormatString(params).c_str());

  // Because this test asserts bounds on scheduling delays it is flaky.  To
  // compensate it loops forever until it passes.  Failures express as test
  // timeouts, in which case the test log can be used to diagnose the issue.
  for (int attempt = 0;; ++attempt) {
    ABEL_RAW_LOG(INFO, "Attempt %d", attempt);

    abel::mutex mu;
    bool value = false;  // condition value (under mu)
    abel::cond_var cv;    // signals a change of `value`

    std::unique_ptr<abel::synchronization_internal::ThreadPool> pool =
        CreateDefaultPool();
    RunAfterDelay(params.satisfy_condition_delay, pool.get(), [&] {
      abel::mutex_lock l(&mu);
      value = true;
      cv.signal();
    });

    abel::mutex_lock lock(&mu);
    abel::abel_time start_time = abel::now();
    abel::duration timeout = params.wait_timeout;
    abel::abel_time deadline = start_time + timeout;
    while (!value) {
      if (params.use_absolute_deadline ? cv.wait_with_deadline(&mu, deadline)
                                       : cv.wait_with_timeout(&mu, timeout)) {
        break;  // deadline/timeout exceeded
      }
      timeout = deadline - abel::now();  // recompute
    }
    bool result = value;  // note: `mu` is still held

    if (DelayIsWithinBounds(params.expected_delay, abel::now() - start_time)) {
      EXPECT_EQ(params.expected_result, result);
      break;
    }
  }
}

TEST(mutex, Logging) {
  // Allow user to look at logging output
  abel::mutex logged_mutex;
  logged_mutex.enable_debug_log("fido_mutex");
  abel::cond_var logged_cv;
  logged_cv.enable_debug_log("rover_cv");
  logged_mutex.lock();
  logged_cv.wait_with_timeout(&logged_mutex, abel::milliseconds(20));
  logged_mutex.unlock();
  logged_mutex.reader_lock();
  logged_mutex.reader_unlock();
  logged_mutex.lock();
  logged_mutex.unlock();
  logged_cv.signal();
  logged_cv.signal_all();
}

// --------------------------------------------------------

// Generate the vector of thread counts for tests parameterized on thread count.
static std::vector<int> AllThreadCountValues() {
  if (kExtendedTest) {
    return {2, 4, 8, 10, 16, 20, 24, 30, 32};
  }
  return {2, 4, 10};
}

// A test fixture parameterized by thread count.
class MutexVariableThreadCountTest : public ::testing::TestWithParam<int> {};

// Instantiate the above with AllThreadCountOptions().
INSTANTIATE_TEST_SUITE_P(ThreadCounts, MutexVariableThreadCountTest,
                         ::testing::ValuesIn(AllThreadCountValues()),
                         ::testing::PrintToStringParamName());

// Reduces iterations by some factor for slow platforms
// (determined empirically).
static int ScaleIterations(int x) {
  // ABEL_MUTEX_READER_LOCK_IS_EXCLUSIVE is set in the implementation
  // of mutex that uses either std::mutex or pthread_mutex_t. Use
  // these as keys to determine the slow implementation.
#if defined(ABEL_MUTEX_READER_LOCK_IS_EXCLUSIVE)
  return x / 10;
#else
  return x;
#endif
}

TEST_P(MutexVariableThreadCountTest, mutex) {
  int threads = GetParam();
  int iterations = ScaleIterations(10000000) / threads;
  int operations = threads * iterations;
  EXPECT_EQ(RunTest(&TestMu, threads, iterations, operations), operations);
#if !defined(ABEL_MUTEX_ENABLE_INVARIANT_DEBUGGING_NOT_IMPLEMENTED)
  iterations = std::min(iterations, 10);
  operations = threads * iterations;
  EXPECT_EQ(RunTestWithInvariantDebugging(&TestMu, threads, iterations,
                                          operations, CheckSumG0G1),
            operations);
#endif
}

TEST_P(MutexVariableThreadCountTest, Try) {
  int threads = GetParam();
  int iterations = 1000000 / threads;
  int operations = iterations * threads;
  EXPECT_EQ(RunTest(&TestTry, threads, iterations, operations), operations);
#if !defined(ABEL_MUTEX_ENABLE_INVARIANT_DEBUGGING_NOT_IMPLEMENTED)
  iterations = std::min(iterations, 10);
  operations = threads * iterations;
  EXPECT_EQ(RunTestWithInvariantDebugging(&TestTry, threads, iterations,
                                          operations, CheckSumG0G1),
            operations);
#endif
}

TEST_P(MutexVariableThreadCountTest, R20ms) {
  int threads = GetParam();
  int iterations = 100;
  int operations = iterations * threads;
  EXPECT_EQ(RunTest(&TestR20ms, threads, iterations, operations), 0);
}

TEST_P(MutexVariableThreadCountTest, RW) {
  int threads = GetParam();
  int iterations = ScaleIterations(20000000) / threads;
  int operations = iterations * threads;
  EXPECT_EQ(RunTest(&TestRW, threads, iterations, operations), operations / 2);
#if !defined(ABEL_MUTEX_ENABLE_INVARIANT_DEBUGGING_NOT_IMPLEMENTED)
  iterations = std::min(iterations, 10);
  operations = threads * iterations;
  EXPECT_EQ(RunTestWithInvariantDebugging(&TestRW, threads, iterations,
                                          operations, CheckSumG0G1),
            operations / 2);
#endif
}

TEST_P(MutexVariableThreadCountTest, Await) {
  int threads = GetParam();
  int iterations = ScaleIterations(500000);
  int operations = iterations;
  EXPECT_EQ(RunTest(&TestAwait, threads, iterations, operations), operations);
}

TEST_P(MutexVariableThreadCountTest, signal_all) {
  int threads = GetParam();
  int iterations = 200000 / threads;
  int operations = iterations;
  EXPECT_EQ(RunTest(&TestSignalAll, threads, iterations, operations),
            operations);
}

TEST(mutex, signal) {
  int threads = 2;  // TestSignal must use two threads
  int iterations = 200000;
  int operations = iterations;
  EXPECT_EQ(RunTest(&TestSignal, threads, iterations, operations), operations);
}

TEST(mutex, Timed) {
  int threads = 10;  // Use a fixed thread count of 10
  int iterations = 1000;
  int operations = iterations;
  EXPECT_EQ(RunTest(&TestCVTimeout, threads, iterations, operations),
            operations);
}

TEST(mutex, CVTime) {
  int threads = 10;  // Use a fixed thread count of 10
  int iterations = 1;
  EXPECT_EQ(RunTest(&TestCVTime, threads, iterations, 1),
            threads * iterations);
}

TEST(mutex, MuTime) {
  int threads = 10;  // Use a fixed thread count of 10
  int iterations = 1;
  EXPECT_EQ(RunTest(&TestMuTime, threads, iterations, 1), threads * iterations);
}

}  // namespace
