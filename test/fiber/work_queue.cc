//
// Created by liyinbin on 2021/4/18.
//


#include "abel/fiber/work_queue.h"

#include "gtest/gtest.h"

#include "abel/fiber/async.h"
#include "testing/fiber.h"
#include "abel/fiber/future.h"
#include "abel/fiber/this_fiber.h"


namespace abel {

    TEST(work_queue, All) {
        testing::RunAsFiber([] {
            int x = 0;  // Not `atomic`.
            auto s = abel::time_now();
            work_queue wq;

            for (int i = 0; i != 10; ++i) {
                wq.Push([&] {
                    ++x;
                    fiber_sleep_for(abel::duration::milliseconds(100));
                });
            }
            EXPECT_LE(abel::time_now() - s, abel::duration::milliseconds(50));
            wq.Stop();
            wq.Join();
            EXPECT_GE(abel::time_now() - s, abel::duration::milliseconds(950));
            EXPECT_EQ(10, x);
        });
    }

    TEST(work_queue, RaceOnExit) {
        testing::RunAsFiber([] {
            std::atomic<std::size_t> finished = 0;
            constexpr auto kWorkers = 100;
            for (int i = 0; i != kWorkers; ++i) {
                fiber_async([&finished] {
                    for (int j = 0; j != 1000; ++j) {
                        int x = 0;  // Not `atomic`.
                        work_queue wq;

                        for (int k = 0; k != 10; ++k) {
                            wq.Push([&] { fiber_blocking_get(fiber_async([&] { ++x; })); });
                        }
                        wq.Stop();
                        wq.Join();
                        ASSERT_EQ(10, x);
                    }
                    ++finished;
                });
            }
            while (finished != kWorkers) {
                fiber_sleep_for(abel::duration::milliseconds(1));
            }
        });
    }

}  // namespace abel
