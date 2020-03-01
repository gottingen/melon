//

#include <abel/synchronization/blocking_counter.h>

#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <gtest/gtest.h>
#include <abel/chrono/clock.h>
#include <abel/chrono/time.h>

namespace abel {

    namespace {

        void PauseAndDecreaseCounter(blocking_counter *counter, int *done) {
            abel::sleep_for(abel::seconds(1));
            *done = 1;
            counter->decrement_count();
        }

        TEST(BlockingCounterTest, BasicFunctionality) {
            // This test verifies that blocking_counter functions correctly. Starts a
            // number of threads that just sleep for a second and decrement a counter.

            // Initialize the counter.
            const int num_workers = 10;
            blocking_counter counter(num_workers);

            std::vector<std::thread> workers;
            std::vector<int> done(num_workers, 0);

            // Start a number of parallel tasks that will just wait for a seconds and
            // then decrement the count.
            workers.reserve(num_workers);
            for (int k = 0; k < num_workers; k++) {
                workers.emplace_back(
                        [&counter, &done, k] { PauseAndDecreaseCounter(&counter, &done[k]); });
            }

            // wait for the threads to have all finished.
            counter.wait();

            // Check that all the workers have completed.
            for (int k = 0; k < num_workers; k++) {
                EXPECT_EQ(1, done[k]);
            }

            for (std::thread &w : workers) {
                w.join();
            }
        }

    }  // namespace

}  // namespace abel
