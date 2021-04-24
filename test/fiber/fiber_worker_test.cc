//
// Created by liyinbin on 2021/4/5.
//

#include "abel/fiber/internal/fiber_worker.h"

#include <deque>
#include <memory>
#include <thread>
#include <vector>

#include "gflags/gflags.h"
#include "gtest/gtest.h"
#include "abel/chrono/clock.h"
#include "abel/base/random.h"
#include "abel/fiber/internal/fiber_entity.h"
#include "abel/fiber/internal/scheduling_group.h"
#include "testing/fiber.h"
#include "abel/fiber/internal/timer_worker.h"

namespace abel {
    namespace fiber_internal {

        class SystemFiberOrNot : public ::testing::TestWithParam<bool> {
        };
#if defined(ABEL_PLATFORM_LINUX)
        TEST_P(SystemFiberOrNot, Affinity) {
            for (int k = 0; k != 1000; ++k) {
                auto sg = std::make_unique<scheduling_group>(std::vector<int>{1, 2, 3}, 16);
                timer_worker dummy(sg.get());
                sg->set_timer_worker(&dummy);
                std::deque<fiber_worker> workers;

                for (int i = 0; i != 16; ++i) {
                    workers.emplace_back(sg.get(), i).start(false);
                }
                testing::StartFiberEntityInGroup(sg.get(), GetParam(), [&] {
                    auto cpu = abel::get_current_processor_id();
                    ASSERT_TRUE(1 <= cpu && cpu <= 3)<<cpu;
                });
                sg->Stop();
                for (auto &&w : workers) {
                    w.join();
                }
            }
        }
#endif
        TEST_P(SystemFiberOrNot, ExecuteFiber) {
            std::atomic<std::size_t> executed{0};
            auto sg = std::make_unique<scheduling_group>(core_affinity(), 16);
            timer_worker dummy(sg.get());
            sg->set_timer_worker(&dummy);
            std::deque<fiber_worker> workers;

            for (int i = 0; i != 16; ++i) {
                workers.emplace_back(sg.get(), i).start(false);
            }
            testing::StartFiberEntityInGroup(sg.get(), GetParam(), [&] {
#if defined(ABEL_PLATFORM_LINUX)
                auto cpu = abel::get_current_processor_id();
                ASSERT_TRUE(1 <= cpu && cpu <= 3);
#endif
                ++executed;
            });
            sg->stop();
            for (auto &&w : workers) {
                w.join();
            }

            ASSERT_EQ(1, executed);
        }

        TEST_P(SystemFiberOrNot, StealFiber) {
            std::atomic<std::size_t> executed{0};
            auto sg = std::make_unique<scheduling_group>(core_affinity(), 16);
            auto sg2 = std::make_unique<scheduling_group>(core_affinity(), 1);
            timer_worker dummy(sg.get());
            sg->set_timer_worker(&dummy);
            std::deque<fiber_worker> workers;

            testing::StartFiberEntityInGroup(sg2.get(), GetParam(), [&] { ++executed; });
            for (int i = 0; i != 16; ++i) {
                auto &&w = workers.emplace_back(sg.get(), i);
                w.add_foreign_scheduling_group(sg2.get(), 1);
                w.start(false);
            }
            while (!executed) {
                // To wake worker up.
                testing::StartFiberEntityInGroup(sg.get(), GetParam(), [] {});
                abel::sleep_for(abel::duration::milliseconds(1));
            }
            sg->stop();
            for (auto &&w : workers) {
                w.join();
            }

            ASSERT_EQ(1, executed);
        }

        INSTANTIATE_TEST_SUITE_P(fiber_worker, SystemFiberOrNot,
                                 ::testing::Values(true, false));

        TEST(fiber_worker, Torture) {
            constexpr auto T = 64;
            // Setting it too large cause `vm.max_map_count` overrun.
            constexpr auto N = 32768;
            constexpr auto P = 128;
            for (int i = 0; i != 50; ++i) {
                std::atomic<std::size_t> executed{0};
                auto sg = std::make_unique<scheduling_group>(core_affinity(), T);
                timer_worker dummy(sg.get());
                sg->set_timer_worker(&dummy);
                std::deque<fiber_worker> workers;

                for (int i = 0; i != T; ++i) {
                    workers.emplace_back(sg.get(), i).start(false);
                }

                // Concurrently create fibers.
                std::thread prods[P];
                for (auto &&t : prods) {
                    t = std::thread([&] {
                        constexpr auto kChildren = 32;
                        static_assert(N % P == 0 && (N / P) % kChildren == 0);
                        for (int i = 0; i != N / P / kChildren; ++i) {
                            testing::StartFiberEntityInGroup(
                                    sg.get(), Random() % 2 == 0 /* system_fiber */, [&] {
                                        ++executed;
                                        for (auto j = 0; j != kChildren - 1 /* minus itself */; ++j) {
                                            testing::StartFiberEntityInGroup(
                                                    sg.get(), Random() % 2 == 0 /* system_fiber */,
                                                    [&] { ++executed; });
                                        }
                                    });
                        }
                    });
                }

                for (auto &&t : prods) {
                    t.join();
                }
                while (executed != N) {
                    //DLOG_INFO("executed: {}", executed);
                    abel::sleep_for(abel::duration::milliseconds(100));
                }
                sg->stop();
                for (auto &&w : workers) {
                    w.join();
                }

                ASSERT_EQ(N, executed);
            }
        }
    }  // namespace fiber_internal

}  // namespace abel
