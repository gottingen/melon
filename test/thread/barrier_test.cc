//

#include <abel/thread/barrier.h>

#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <gtest/gtest.h>
#include <abel/thread/mutex.h>
#include <abel/chrono/clock.h>


TEST(barrier, SanityTest) {
    constexpr int kNumThreads = 10;
    abel::barrier *barrier = new abel::barrier(kNumThreads);

    abel::mutex mutex;
    int counter = 0;  // Guarded by mutex.

    auto thread_func = [&] {
        if (barrier->block()) {
            // This thread is the last thread to reach the barrier so it is
            // responsible for deleting it.
            delete barrier;
        }

        // Increment the counter.
        abel::mutex_lock lock(&mutex);
        ++counter;
    };

    // Start (kNumThreads - 1) threads running thread_func.
    std::vector<std::thread> threads;
    for (int i = 0; i < kNumThreads - 1; ++i) {
        threads.push_back(std::thread(thread_func));
    }

    // Give (kNumThreads - 1) threads a chance to reach the barrier.
    // This test assumes at least one thread will have run after the
    // sleep has elapsed. Sleeping in a test is usually bad form, but we
    // need to make sure that we are testing the barrier instead of some
    // other synchronization method.
    abel::sleep_for(abel::seconds(1));

    // The counter should still be zero since no thread should have
    // been able to pass the barrier yet.
    {
        abel::mutex_lock lock(&mutex);
        EXPECT_EQ(counter, 0);
    }

    // Start 1 more thread. This should make all threads pass the barrier.
    threads.push_back(std::thread(thread_func));

    // All threads should now be able to proceed and finish.
    for (auto &thread : threads) {
        thread.join();
    }

    // All threads should now have incremented the counter.
    abel::mutex_lock lock(&mutex);
    EXPECT_EQ(counter, kNumThreads);
}
