//
// Created by liyinbin on 2021/4/18.
//


#include "abel/fiber/fiber.h"

#include <atomic>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "abel/thread/numa.h"
#include "abel/base/random.h"
#include "abel/fiber/fiber_local.h"
#include "abel/fiber/fiber_latch.h"
#include "abel/fiber/runtime.h"
#include "abel/fiber/this_fiber.h"
#include "abel/fiber/fiber_config.h"

using namespace std::literals;

namespace abel {

    namespace {

        template<class F>
        void run_as_fiber(F &&f) {
            start_runtime();
            std::atomic<bool> done{};
            fiber([&, f = std::forward<F>(f)] {
                f();
                done = true;
            }).detach();
            while (!done) {
                std::this_thread::sleep_for(1ms);
            }
            terminate_runtime();
        }

    }  // namespace

    TEST(fiber, StartWithDispatch) {
        fiber_config::get_global_fiber_config().fiber_stack_enable_guard_page = false;
        fiber_config::get_global_fiber_config().fiber_run_queue_size = 1048576;

        run_as_fiber([] {
            for (int k = 0; k != 10; ++k) {
                constexpr auto N = 10000;

                std::atomic<std::size_t> run{};
                std::vector<fiber> fs(N);

                for (int i = 0; i != N; ++i) {
                    fs[i] = fiber([&] {
                        auto we_re_in = std::this_thread::get_id();
                        fiber(fiber_internal::Launch::Dispatch, [&, we_re_in] {
                            ASSERT_EQ(we_re_in, std::this_thread::get_id());
                            ++run;
                        }).detach();
                    });
                }

                while (run != N) {
                    std::this_thread::sleep_for(1ms);
                }

                for (auto &&e : fs) {
                    ASSERT_TRUE(e.joinable());
                    e.join();
                }

                ASSERT_EQ(N, run);
            }
        });
    }

    TEST(fiber, SchedulingGroupLocal) {
        fiber_config::get_global_fiber_config().fiber_stack_enable_guard_page = false;
        fiber_config::get_global_fiber_config().fiber_run_queue_size = 1048576;

        run_as_fiber([] {
            constexpr auto N = 10000;

            std::atomic<std::size_t> run{};
            std::vector<fiber> fs(N);
            std::atomic<bool> stop{};

            for (std::size_t i = 0; i != N; ++i) {
                auto sgi = i % get_scheduling_group_count();
                auto cb = [&, sgi] {
                    while (!stop) {
                        ASSERT_EQ(sgi, fiber_internal::nearest_scheduling_group_index());
                        fiber_yield();
                    }
                    ++run;
                    DLOG_INFO("{}", run);
                };
                fs[i] = fiber(fiber::attributes{.scheduling_group = sgi,
                                      .scheduling_group_local = true},
                              cb);
            }

            auto start = abel::time_now();

// 10s should be far from enough. In my test the assertion above fires
// almost immediately if `scheduling_group_local` is not set.
            while (start + abel::duration::seconds(10) >  abel::time_now()) {
                std::this_thread::sleep_for(1ms);

// Wake up workers in each scheduling group (for them to be thieves.).
                auto count = get_scheduling_group_count();
                for (std::size_t i = 0; i != count; ++i) {
                    fiber(fiber::attributes{.scheduling_group = i}, [] {}).join();
                }
            }

            stop = true;
            for (auto &&e : fs) {
                ASSERT_TRUE(e.joinable());
                e.join();
            }

            ASSERT_EQ(N, run);
        });
    }

    TEST(fiber, WorkStealing) {
        if (numa::get_available_nodes().size() == 1) {
            DLOG_INFO("Non-NUMA system, ignored.");
            return;
        }
        fiber_config::get_global_fiber_config().fiber_stack_enable_guard_page = false;
        fiber_config::get_global_fiber_config().cross_numa_work_stealing_ratio = 1;

        run_as_fiber([] {
            std::atomic<bool> stealing_happened{};
            constexpr auto N = 10000;

            std::atomic<std::size_t> run{};
            std::vector<fiber> fs(N);

            for (int i = 0; i != N; ++i) {
                fiber::attributes attr{.scheduling_group =
                i % get_scheduling_group_count()};
                auto cb = [&] {
                    auto start_node = numa::get_current_node();
                    while (!stealing_happened) {
                        if (start_node != numa::get_current_node()) {
                            DLOG_INFO("Start on node #{}, running on node #{} now.",
                                           start_node, numa::get_current_node());
                            stealing_happened = true;
                        } else {
                            fiber_sleep_for(abel::duration::microseconds(1));
                        }
                    }
                    ++run;
                };
                fs[i] = fiber(attr, cb);
            }

            while (run != N) {
                std::this_thread::sleep_for(1ms);

                auto count = get_scheduling_group_count();
                for (std::size_t i = 0; i != count; ++i) {
                    fiber(fiber::attributes{.scheduling_group = i}, [] {}).join();
                }
            }

            for (auto &&e : fs) {
                ASSERT_TRUE(e.joinable());
                e.join();
            }

            ASSERT_EQ(N, run);
            ASSERT_TRUE(stealing_happened);
        });
    }

    TEST(fiber, BatchStart) {
        run_as_fiber([&] {
            static constexpr auto N = 10;
            static constexpr auto B = 10000;
            std::atomic<std::size_t> started{};

            for (int i = 0; i != N; ++i) {
                std::atomic<std::size_t> done{};
                std::vector<function < void()>>
                procs;
                for (int j = 0; j != B; ++j) {
                    procs.push_back([&] {
                        ++started;
                        ++done;
                        DLOG_INFO("done: {} started: {}", done, started);
                    });
                }
                fiber_internal::batch_start_fiber_detached(std::move(procs));
                while (done != B) {
                }
            }

            ASSERT_EQ(N * B, started.load());
        });
    }

    TEST(fiber, start_fiber_from_pthread) {
        run_as_fiber([&] {
            std::atomic<bool> called{};
            std::thread([&] {
                start_fiber_from_pthread([&] {
                    fiber_yield();  // Would crash in pthread.
                    ASSERT_TRUE(1);
                    called = true;
                });
            }).join();
            while (!called.load()) {
// NOTHING.
            }
        });
    }

}  // namespace abel
