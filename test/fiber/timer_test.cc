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

    TEST(Timer, SetTimer) {
        testing::RunAsFiber([] {
            auto start = abel::time_now();
            std::atomic<bool> done{};
            auto timer_id = SetTimer(start + 100 *one_mill, [&](auto) {
                ASSERT_NEAR((abel::time_now() - start) / one_mill, 100 * one_mill / one_mill, 10);
                done = true;
            });
            while (!done) {
                abel::sleep_for(one_mill);
            }
            KillTimer(timer_id);
        });
    }

    TEST(Timer, SetPeriodicTimer) {
        testing::RunAsFiber([] {
            auto start = abel::time_now();
            std::atomic<std::size_t> called{};
            auto timer_id = SetTimer(start + 100* one_mill, 10 *one_mill, [&](auto) {
                ASSERT_NEAR((abel::time_now() - start) / one_mill,
                            (100 *one_mill + called.load() * 10 * one_mill) / one_mill, 10);
                ++called;
            });
            while (called != 10) {
                abel::sleep_for(one_mill);
            }
            KillTimer(timer_id);

            // It's possible that the timer callback is running when `KillTimer` is
            // called, so wait for it to complete.
            abel::sleep_for(500 * one_mill);
        });
    }

    TEST(Timer, TimerKiller) {
        testing::RunAsFiber([] {
            auto start = abel::time_now();
            std::atomic<bool> done{};
            TimerKiller killer(SetTimer(start + 100 * one_mill, [&](auto) {
                ASSERT_NEAR((abel::time_now() - start) / one_mill, 100 * one_mill / one_mill, 10);
                done = true;
            }));
            while (!done) {
                abel::sleep_for(one_mill);
            }
// We rely on heap checker here to ensure the timer is not leaked.
        });
    }

    TEST(Timer, SetDetachedTimer) {
        testing::RunAsFiber([] {
            auto start = abel::time_now();
            std::atomic<bool> called{};
            SetDetachedTimer(start + 100 * one_mill, [&]() {
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
