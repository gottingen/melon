//

#include <abel/synchronization/notification.h>

#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <gtest/gtest.h>
#include <abel/synchronization/mutex.h>

namespace abel {
ABEL_NAMESPACE_BEGIN

// A thread-safe class that holds a counter.
class ThreadSafeCounter {
 public:
  ThreadSafeCounter() : count_(0) {}

  void Increment() {
    mutex_lock lock(&mutex_);
    ++count_;
  }

  int Get() const {
    mutex_lock lock(&mutex_);
    return count_;
  }

  void WaitUntilGreaterOrEqual(int n) {
    mutex_lock lock(&mutex_);
    auto cond = [this, n]() { return count_ >= n; };
    mutex_.Await(condition(&cond));
  }

 private:
  mutable mutex mutex_;
  int count_;
};

// Runs the |i|'th worker thread for the tests in BasicTests().  Increments the
// |ready_counter|, waits on the |notification|, and then increments the
// |done_counter|.
static void RunWorker(int i, ThreadSafeCounter* ready_counter,
                      notification* notification,
                      ThreadSafeCounter* done_counter) {
  ready_counter->Increment();
  notification->wait_for_notification();
  done_counter->Increment();
}

// Tests that the |notification| properly blocks and awakens threads.  Assumes
// that the |notification| is not yet triggered.  If |notify_before_waiting| is
// true, the |notification| is triggered before any threads are created, so the
// threads never block in wait_for_notification().  Otherwise, the |notification|
// is triggered at a later point when most threads are likely to be blocking in
// wait_for_notification().
static void BasicTests(bool notify_before_waiting, notification* notification) {
  EXPECT_FALSE(notification->has_been_notified());
  EXPECT_FALSE(
      notification->wait_for_notification_with_timeout(abel::milliseconds(0)));
  EXPECT_FALSE(notification->wait_for_notification_with_deadline(abel::now()));

  const abel::duration delay = abel::milliseconds(50);
  const abel::abel_time start = abel::now();
  EXPECT_FALSE(notification->wait_for_notification_with_timeout(delay));
  const abel::duration elapsed = abel::now() - start;

  // Allow for a slight early return, to account for quality of implementation
  // issues on various platforms.
  const abel::duration slop = abel::microseconds(200);
  EXPECT_LE(delay - slop, elapsed)
      << "wait_for_notification_with_timeout returned " << delay - elapsed
      << " early (with " << slop << " slop), start time was " << start;

  ThreadSafeCounter ready_counter;
  ThreadSafeCounter done_counter;

  if (notify_before_waiting) {
    notification->Notify();
  }

  // Create a bunch of threads that increment the |done_counter| after being
  // notified.
  const int kNumThreads = 10;
  std::vector<std::thread> workers;
  for (int i = 0; i < kNumThreads; ++i) {
    workers.push_back(std::thread(&RunWorker, i, &ready_counter, notification,
                                  &done_counter));
  }

  if (!notify_before_waiting) {
    ready_counter.WaitUntilGreaterOrEqual(kNumThreads);

    // Workers have not been notified yet, so the |done_counter| should be
    // unmodified.
    EXPECT_EQ(0, done_counter.Get());

    notification->Notify();
  }

  // After notifying and then joining the workers, both counters should be
  // fully incremented.
  notification->wait_for_notification();  // should exit immediately
  EXPECT_TRUE(notification->has_been_notified());
  EXPECT_TRUE(notification->wait_for_notification_with_timeout(abel::seconds(0)));
  EXPECT_TRUE(notification->wait_for_notification_with_deadline(abel::now()));
  for (std::thread& worker : workers) {
    worker.join();
  }
  EXPECT_EQ(kNumThreads, ready_counter.Get());
  EXPECT_EQ(kNumThreads, done_counter.Get());
}

TEST(NotificationTest, SanityTest) {
  notification local_notification1, local_notification2;
  BasicTests(false, &local_notification1);
  BasicTests(true, &local_notification2);
}

ABEL_NAMESPACE_END
}  // namespace abel
