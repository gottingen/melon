// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com



#include <atomic>
#include <condition_variable>  // NOLINT(build/c++11)
#include <functional>
#include <limits>
#include <mutex>               // NOLINT(build/c++11)
#include <string>
#include <thread>              // NOLINT(build/c++11)
#include "abel/thread/internal/per_thread_sem.h"
#include "gtest/gtest.h"
#include "abel/chrono/internal/cycle_clock.h"
#include "abel/thread/internal/thread_identity.h"
#include "abel/strings/str_cat.h"
#include "abel/chrono/clock.h"
#include "abel/chrono/time.h"

// In this test we explicitly avoid the use of synchronization
// primitives which might use per_thread_sem, most notably abel::mutex.

namespace abel {

namespace thread_internal {

class SimpleSemaphore {
  public:
    SimpleSemaphore() : count_(0) {}

    // Decrements (locks) the semaphore. If the semaphore's value is
    // greater than zero, then the decrement proceeds, and the function
    // returns, immediately. If the semaphore currently has the value
    // zero, then the call blocks until it becomes possible to perform
    // the decrement.
    void wait() {
        std::unique_lock<std::mutex> lock(mu_);
        cv_.wait(lock, [this]() { return count_ > 0; });
        --count_;
        cv_.notify_one();
    }

    // Increments (unlocks) the semaphore. If the semaphore's value
    // consequently becomes greater than zero, then another thread
    // blocked wait() call will be woken up and proceed to lock the
    // semaphore.
    void post() {
        std::lock_guard<std::mutex> lock(mu_);
        ++count_;
        cv_.notify_one();
    }

  private:
    std::mutex mu_;
    std::condition_variable cv_;
    int count_;
};

struct ThreadData {
    int num_iterations;                 // Number of replies to send.
    SimpleSemaphore identity2_written;  // Posted by thread writing identity2.
    thread_internal::thread_identity *identity1;  // First post()-er.
    thread_internal::thread_identity *identity2;  // First wait()-er.
    kernel_timeout timeout;
};

// Need friendship with per_thread_sem.
class PerThreadSemTest : public testing::Test {
  public:
    static void TimingThread(ThreadData *t) {
        t->identity2 = get_or_create_current_thread_identity();
        t->identity2_written.post();
        while (t->num_iterations--) {
            wait(t->timeout);
            post(t->identity1);
        }
    }

    void TestTiming(const char *msg, bool timeout) {
        static const int kNumIterations = 100;
        ThreadData t;
        t.num_iterations = kNumIterations;
        t.timeout = timeout ?
                    kernel_timeout(abel::now() + abel::duration::seconds(10000))  // far in the future
                            : kernel_timeout::never();
        t.identity1 = get_or_create_current_thread_identity();

        // We can't use the Thread class here because it uses the mutex
        // class which will invoke per_thread_sem, so we use std::thread instead.
        std::thread partner_thread(std::bind(TimingThread, &t));

        // wait for our partner thread to register their identity.
        t.identity2_written.wait();

        int64_t min_cycles = std::numeric_limits<int64_t>::max();
        int64_t total_cycles = 0;
        for (int i = 0; i < kNumIterations; ++i) {
            abel::sleep_for(abel::duration::milliseconds(20));
            int64_t cycles = abel::chrono_internal::cycle_clock::now();
            post(t.identity2);
            wait(t.timeout);
            cycles = chrono_internal::cycle_clock::now() - cycles;
            min_cycles = std::min(min_cycles, cycles);
            total_cycles += cycles;
        }
        std::string out = string_cat(
                msg, "min cycle count=", min_cycles, " avg cycle count=",
                abel::SixDigits(static_cast<double>(total_cycles) / kNumIterations));
        printf("%s\n", out.c_str());

        partner_thread.join();
    }

  protected:
    static void post(thread_internal::thread_identity *id) {
        per_thread_sem::post(id);
    }

    static bool wait(kernel_timeout t) {
        return per_thread_sem::wait(t);
    }

    // convenience overload
    static bool wait(abel::abel_time t) {
        return wait(kernel_timeout(t));
    }

    static void tick(thread_internal::thread_identity *identity) {
        per_thread_sem::tick(identity);
    }
};

namespace {

TEST_F(PerThreadSemTest, WithoutTimeout) {
    PerThreadSemTest::TestTiming("Without timeout: ", false);
}

TEST_F(PerThreadSemTest, WithTimeout) {
    PerThreadSemTest::TestTiming("With timeout:    ", true);
}

TEST_F(PerThreadSemTest, Timeouts) {
    const abel::duration delay = abel::duration::milliseconds(50);
    const abel::abel_time start = abel::now();
    EXPECT_FALSE(wait(start + delay));
    const abel::duration elapsed = abel::now() - start;
    // Allow for a slight early return, to account for quality of implementation
    // issues on various platforms.
    const abel::duration slop = abel::duration::microseconds(200);
    EXPECT_LE(delay - slop, elapsed)
                        << "wait returned " << delay - elapsed
                        << " early (with " << slop << " slop), start time was " << start;

    abel::abel_time negative_timeout = abel::abel_time::unix_epoch() - abel::duration::milliseconds(100);
    EXPECT_FALSE(wait(negative_timeout));
    EXPECT_LE(negative_timeout, abel::now() + slop);  // trivially true :)

    post(get_or_create_current_thread_identity());
    // The wait here has an expired timeout, but we have a wake to consume,
    // so this should succeed
    EXPECT_TRUE(wait(negative_timeout));
}

}  // namespace

}  // namespace thread_internal

}  // namespace abel
