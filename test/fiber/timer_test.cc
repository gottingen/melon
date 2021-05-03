//
// Created by liyinbin on 2021/4/18.
//


#include "abel/fiber/timer.h"

#include <atomic>
#include <thread>

#include "gtest/gtest.h"

#include "testing/fiber.h"
#include "abel/fiber/this_fiber.h"


namespace abel {

    auto one_mill = abel::duration::milliseconds(1);

    TEST(Timer, set_timer) {
        testing::run_as_fiber([] {
            auto start = abel::time_now();
            std::atomic<bool> done{};
            auto timer_id = set_timer(start + 100 *one_mill, [&](auto) {
                ASSERT_NEAR((abel::time_now() - start) / one_mill, 100 * one_mill / one_mill, 10);
                done = true;
            });
            while (!done) {
                abel::sleep_for(one_mill);
            }
            stop_timer(timer_id);
        });
    }

    TEST(Timer, SetPeriodicTimer) {
        testing::run_as_fiber([] {
            auto start = abel::time_now();
            std::atomic<std::size_t> called{};
            auto timer_id = set_timer(start + 100* one_mill, 10 *one_mill, [&](auto) {
                ASSERT_NEAR((abel::time_now() - start) / one_mill,
                            (100 *one_mill + called.load() * 10 * one_mill) / one_mill, 10);
                ++called;
            });
            while (called != 10) {
                abel::sleep_for(one_mill);
            }
            stop_timer(timer_id);

            // It's possible that the timer callback is running when `stop_timer` is
            // called, so wait for it to complete.
            abel::sleep_for(500 * one_mill);
        });
    }

    TEST(Timer, timer_killer) {
        testing::run_as_fiber([] {
            auto start = abel::time_now();
            std::atomic<bool> done{};
            timer_killer killer(set_timer(start + 100 * one_mill, [&](auto) {
                ASSERT_NEAR((abel::time_now() - start) / one_mill, 100 * one_mill / one_mill, 10);
                done = true;
            }));
            while (!done) {
                abel::sleep_for(one_mill);
            }
// We rely on heap checker here to ensure the timer is not leaked.
        });
    }

    TEST(Timer, set_detached_timer) {
        testing::run_as_fiber([] {
            auto start = abel::time_now();
            std::atomic<bool> called{};
            set_detached_timer(start + 100 * one_mill, [&]() {
                ASSERT_NEAR((abel::time_now() - start) / one_mill, 100 * one_mill / one_mill, 10);
                called = true;
            });
            while (!called) {
                abel::sleep_for(one_mill);
            }
        });
// No leak should be reported.
    }

}  // namespace abel
