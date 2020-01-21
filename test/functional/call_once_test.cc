//

#include <abel/functional/call_once.h>

#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <abel/base/profile.h>
#include <abel/base/const_init.h>
#include <abel/threading/thread_annotations.h>
#include <abel/synchronization/mutex.h>

namespace abel {

namespace {

abel::once_flag once;

ABEL_CONST_INIT mutex counters_mu(abel::kConstInit);

int running_thread_count ABEL_GUARDED_BY(counters_mu) = 0;
int call_once_invoke_count ABEL_GUARDED_BY(counters_mu) = 0;
int call_once_finished_count ABEL_GUARDED_BY(counters_mu) = 0;
int call_once_return_count ABEL_GUARDED_BY(counters_mu) = 0;
bool done_blocking ABEL_GUARDED_BY(counters_mu) = false;

// Function to be called from abel::call_once.  Waits for a notification.
void WaitAndIncrement() {
  counters_mu.lock();
  ++call_once_invoke_count;
  counters_mu.unlock();

  counters_mu.LockWhen(condition(&done_blocking));
  ++call_once_finished_count;
  counters_mu.unlock();
}

void ThreadBody() {
  counters_mu.lock();
  ++running_thread_count;
  counters_mu.unlock();

  abel::call_once(once, WaitAndIncrement);

  counters_mu.lock();
  ++call_once_return_count;
  counters_mu.unlock();
}

// Returns true if all threads are set up for the test.
bool ThreadsAreSetup(void*) ABEL_EXCLUSIVE_LOCKS_REQUIRED(counters_mu) {
  // All ten threads must be running, and WaitAndIncrement should be blocked.
  return running_thread_count == 10 && call_once_invoke_count == 1;
}

TEST(CallOnceTest, ExecutionCount) {
  std::vector<std::thread> threads;

  // Start 10 threads all calling call_once on the same once_flag.
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back(ThreadBody);
  }


  // wait until all ten threads have started, and WaitAndIncrement has been
  // invoked.
  counters_mu.LockWhen(condition(ThreadsAreSetup, nullptr));

  // WaitAndIncrement should have been invoked by exactly one call_once()
  // instance.  That thread should be blocking on a notification, and all other
  // call_once instances should be blocking as well.
  EXPECT_EQ(call_once_invoke_count, 1);
  EXPECT_EQ(call_once_finished_count, 0);
  EXPECT_EQ(call_once_return_count, 0);

  // Allow WaitAndIncrement to finish executing.  Once it does, the other
  // call_once waiters will be unblocked.
  done_blocking = true;
  counters_mu.unlock();

  for (std::thread& thread : threads) {
    thread.join();
  }

  counters_mu.lock();
  EXPECT_EQ(call_once_invoke_count, 1);
  EXPECT_EQ(call_once_finished_count, 1);
  EXPECT_EQ(call_once_return_count, 10);
  counters_mu.unlock();
}

}  // namespace

}  // namespace abel
