//
// Created by liyinbin on 2021/4/18.
//


#include "abel/fiber/fiber_local.h"

#include <atomic>
#include <thread>
#include <vector>

#include "gflags/gflags.h"
#include "gtest/gtest.h"

#include "abel/base/random.h"
#include "testing/fiber.h"
#include "abel/fiber/fiber.h"
#include "abel/fiber/this_fiber.h"

using namespace std::literals;

DECLARE_bool(fiber_stack_enable_guard_page);

namespace abel {

    TEST(ThisFiber, fiber_yield) {
        FLAGS_fiber_stack_enable_guard_page = false;

        testing::RunAsFiber([] {
            for (int k = 0; k != 10; ++k) {
                constexpr auto N = 10000;

                std::atomic<std::size_t> run{};
                std::atomic<bool> ever_switched_thread{};
                std::vector<fiber> fs(N);

                for (int i = 0; i != N; ++i) {
                    fs[i] = fiber([&run, &ever_switched_thread] {
                        // `Yield()`
                        auto tid = std::this_thread::get_id();
                        while (tid == std::this_thread::get_id()) {
                            fiber_yield();
                        }
                        ever_switched_thread = true;
                        ++run;
                    });
                }

                for (auto &&e : fs) {
                    ASSERT_TRUE(e.joinable());
                    e.join();
                }

                ASSERT_EQ(N, run);
                ASSERT_TRUE(ever_switched_thread);
            }
        });
    }

    TEST(ThisFiber, sleep_x) {
        testing::RunAsFiber([] {
            for (int k = 0; k != 10; ++k) {
                // Don't run too many fibers here, waking pthread worker up is costly and
                // incurs delay. With too many fibers, that delay fails the UT (we're
                // testing timer delay here).
                constexpr auto N = 100;

                std::atomic<std::size_t> run{};
                std::vector<fiber> fs(N);

                for (int i = 0; i != N; ++i) {
                    fs[i] = fiber([&run] {
                        // `SleepFor()`
                        auto sleep_for = abel::duration::milliseconds(Random(100));
                        auto start = abel::time_now();  // Used system_clock intentionally.
                        fiber_sleep_for(sleep_for);
                        ASSERT_NEAR((abel::time_now() - start) / abel::duration::milliseconds(1),
                                    sleep_for / abel::duration::milliseconds(1), 30);

                        // `SleepUntil()`
                        auto sleep_until = abel::time_now() + abel::duration::milliseconds(Random(100));
                        fiber_sleep_until(sleep_until);
                        ASSERT_NEAR((abel::time_now().to_duration()) / abel::duration::milliseconds(1),
                                    sleep_until.to_duration() / abel::duration::milliseconds(1), 30);

                        ++run;
                    });
                }

                for (auto &&e : fs) {
                    ASSERT_TRUE(e.joinable());
                    e.join();
                }

                ASSERT_EQ(N, run);
            }
        });
    }

}  // namespace abel
