//
// Created by liyinbin on 2021/4/12.
//


#include "abel/fiber/internal/waitable.h"

#include <atomic>
#include <deque>
#include <memory>
#include <numeric>
#include <queue>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "abel/base/random.h"
#include "abel/fiber/internal/fiber_entity.h"
#include "abel/fiber/internal/fiber_worker.h"
#include "abel/fiber/internal/scheduling_group.h"
#include "testing/fiber.h"
#include "abel/fiber/internal/timer_worker.h"

namespace abel {
    namespace fiber_internal {

        namespace {

            void Sleep(abel::duration ns) {
                waitable_timer wt(abel::time_now() + ns);
                wt.wait();
            }

            void RandomDelay() {
                auto round = Random(100);
                for (int i = 0; i != round; ++i) {
                    abel::time_now();
                }
            }

        }  // namespace

        class SystemFiberOrNot : public ::testing::TestWithParam<bool> {
        };

        // **Concurrently** call `cb` for `times`.
        template<class F>
        void RunInFiber(std::size_t times, bool system_fiber, F cb) {
            std::atomic<std::size_t> called{};
            auto sg = std::make_unique<scheduling_group>(core_affinity(), 16);
            timer_worker timer_worker(sg.get());
            sg->set_timer_worker(&timer_worker);
            std::deque<fiber_worker> workers;

            for (int i = 0; i != 16; ++i) {
                workers.emplace_back(sg.get(), i).start(false);
            }
            timer_worker.start();
            for (int i = 0; i != times; ++i) {
                testing::start_fiber_entity_in_group(sg.get(), system_fiber, [&, i] {
                    cb(i);
                    ++called;
                });
            }
            while (called != times) {
                abel::sleep_for(abel::duration::milliseconds(100));
            }
            sg->stop();
            timer_worker.stop();
            for (auto &&w : workers) {
                w.join();
            }
            timer_worker.join();
        }

        TEST_P(SystemFiberOrNot, WaitableTimer) {
            RunInFiber(100, GetParam(), [](auto) {
                auto now = abel::time_now();
                waitable_timer wt(now + abel::duration::seconds(1));
                wt.wait();
                EXPECT_NEAR(abel::duration::seconds(1) / abel::duration::milliseconds(1),
                            (abel::time_now() - now) / abel::duration::milliseconds(1), 100);
            });
        }

        TEST_P(SystemFiberOrNot, fiber_mutex) {
            for (int i = 0; i != 10; ++i) {
                fiber_mutex m;
                int value = 0;
                RunInFiber(10000, GetParam(), [&](auto) {
                    std::scoped_lock _(m);
                    ++value;
                });
                ASSERT_EQ(10000, value);
            }
        }

        TEST_P(SystemFiberOrNot, fiber_cond) {
            constexpr auto N = 10000;

            for (int i = 0; i != 10; ++i) {
                fiber_mutex m[N];
                fiber_cond cv[N];
                std::queue<int> queue[N];
                std::atomic<std::size_t> read{}, write{};
                int sum[N] = {};

                // We, in fact, are passing data between two scheduling groups.
                //
                // This should work anyway.
                auto prods = std::thread([&] {
                    RunInFiber(N, GetParam(), [&](auto index) {
                        auto to = Random(N - 1);
                        std::scoped_lock _(m[to]);
                        queue[to].push(index);
                        cv[to].notify_one();
                        ++write;
                    });
                });
                auto signalers = std::thread([&] {
                    RunInFiber(N, GetParam(), [&](auto index) {
                        std::unique_lock lk(m[index]);
                        bool exit = false;
                        while (!exit) {
                            auto &&mq = queue[index];
                            cv[index].wait(lk, [&] { return !mq.empty(); });
                            ASSERT_TRUE(lk.owns_lock());
                            while (!mq.empty()) {
                                if (mq.front() == -1) {
                                    exit = true;
                                    break;
                                }
                                sum[index] += mq.front();
                                ++read;
                                mq.pop();
                            }
                        }
                    });
                });
                prods.join();
                RunInFiber(N, GetParam(), [&](auto index) {
                    std::scoped_lock _(m[index]);
                    queue[index].push(-1);
                    cv[index].notify_one();
                });
                signalers.join();
                ASSERT_EQ((N - 1) * N / 2,
                          std::accumulate(std::begin(sum), std::end(sum), 0));
            }
        }

        TEST_P(SystemFiberOrNot, ConditionVariable2) {
            constexpr auto N = 1000;

            for (int i = 0; i != 50; ++i) {
                fiber_mutex m[N];
                fiber_cond cv[N];
                bool f[N] = {};
                std::atomic<int> sum{};
                auto prods = std::thread([&] {
                    RunInFiber(N, GetParam(), [&](auto index) {
                        Sleep(Random(10) * abel::duration::milliseconds(1));
                        std::scoped_lock _(m[index]);
                        f[index] = true;
                        cv[index].notify_one();
                    });
                });
                auto signalers = std::thread([&] {
                    RunInFiber(N, GetParam(), [&](auto index) {
                        Sleep(Random(10) * abel::duration::milliseconds(1));
                        std::unique_lock lk(m[index]);
                        cv[index].wait(lk, [&] { return f[index]; });
                        ASSERT_TRUE(lk.owns_lock());
                        sum += index;
                    });
                });
                prods.join();
                signalers.join();
                ASSERT_EQ((N - 1) * N / 2, sum);
            }
        }


        TEST_P(SystemFiberOrNot, ConditionVariableTimeout) {
            constexpr auto N = 1000;
            std::atomic<std::size_t> timed_out{};
            fiber_mutex m;
            fiber_cond cv;
            RunInFiber(N, GetParam(), [&](auto index) {
                std::unique_lock lk(m);
                timed_out += !cv.wait_until(lk, abel::time_now() + abel::duration::milliseconds(1));
            });
            ASSERT_EQ(N, timed_out);
        }

        TEST_P(SystemFiberOrNot, ConditionVariableRace) {
            constexpr auto N = 1000;

            for (int i = 0; i != 5; ++i) {
                fiber_mutex m;
                fiber_cond cv;
                std::atomic<int> sum{};
                auto prods = std::thread([&] {
                    RunInFiber(N, GetParam(), [&](auto index) {
                        for (int j = 0; j != 100; ++j) {
                            Sleep(Random(100) * abel::duration::microseconds(1));
                            std::scoped_lock _(m);
                            cv.notify_all();
                        }
                    });
                });
                auto signalers = std::thread([&] {
                    RunInFiber(N, GetParam(), [&](auto index) {
                        for (int j = 0; j != 100; ++j) {
                            std::unique_lock lk(m);
                            cv.wait_until(lk, abel::time_now() + abel::duration::microseconds(50));
                            ASSERT_TRUE(lk.owns_lock());
                        }
                        sum += index;
                    });
                });
                prods.join();
                signalers.join();
                ASSERT_EQ((N - 1) * N / 2, sum);
            }
        }

        TEST_P(SystemFiberOrNot, exit_barrier) {
            constexpr auto N = 10000;

            for (int i = 0; i != 10; ++i) {
                std::deque<exit_barrier> l;
                std::atomic<std::size_t> waited{};

                for (int j = 0; j != N; ++j) {
                    l.emplace_back();
                }

                auto counters = std::thread([&] {
                    RunInFiber(N, GetParam(), [&](auto index) {
                        Sleep(Random(10) * abel::duration::milliseconds(1));
                        l[index].unsafe_count_down(l[index].GrabLock());
                    });
                });
                auto waiters = std::thread([&] {
                    RunInFiber(N, GetParam(), [&](auto index) {
                        Sleep(Random(10) * abel::duration::milliseconds(1));
                        l[index].wait();
                        ++waited;
                    });
                });
                counters.join();
                waiters.join();
                ASSERT_EQ(N, waited);
            }
        }

        TEST_P(SystemFiberOrNot, Event) {
            constexpr auto N = 10000;

            for (int i = 0; i != 10; ++i) {
                std::deque<wait_event> evs;
                std::atomic<std::size_t> waited{};

                for (int j = 0; j != N; ++j) {
                    evs.emplace_back();
                }

                auto counters = std::thread([&] {
                    RunInFiber(N, GetParam(), [&](auto index) {
                        RandomDelay();
                        evs[index].set();
                    });
                });
                auto waiters = std::thread([&] {
                    RunInFiber(N, GetParam(), [&](auto index) {
                        RandomDelay();
                        evs[index].wait();
                        ++waited;
                    });
                });
                counters.join();
                waiters.join();
                ASSERT_EQ(N, waited);
            }
        }

        TEST_P(SystemFiberOrNot, oneshot_timed_event) {
            RunInFiber(1, GetParam(), [&](auto index) {
                oneshot_timed_event ev1(abel::time_now() + abel::duration::milliseconds(1000)),
                        ev2(abel::time_now() + abel::duration::milliseconds(10));

                auto start = abel::time_now();
                ev2.wait();
                EXPECT_LT((abel::time_now() - start) / abel::duration::milliseconds(1), 100);

                auto t = std::thread([&] {
                    abel::sleep_for(abel::duration::milliseconds(500));
                    ev1.set();
                });
                start = abel::time_now();
                ev1.wait();
                EXPECT_NEAR((abel::time_now() - start) / abel::duration::milliseconds(1), 500, 100);
                t.join();
            });
        }

        TEST_P(SystemFiberOrNot, OneshotTimedEventTorture) {
            constexpr auto N = 10000;

            RunInFiber(1, GetParam(), [&](auto) {
                for (int i = 0; i != 10; ++i) {
                    std::deque<oneshot_timed_event> evs;
                    std::atomic<std::size_t> waited{};

                    for (int j = 0; j != N; ++j) {
                        evs.emplace_back(abel::time_now() + Random(1000) * abel::duration::milliseconds(1));
                    }
                    auto counters = std::thread([&] {
                        RunInFiber(N, GetParam(), [&](auto index) {
                            RandomDelay();
                            evs[index].set();
                        });
                    });
                    auto waiters = std::thread([&] {
                        RunInFiber(N, GetParam(), [&](auto index) {
                            RandomDelay();
                            evs[index].wait();
                            ++waited;
                        });
                    });
                    counters.join();
                    waiters.join();
                    ASSERT_EQ(N, waited);
                }
            });
        }

        TEST_P(SystemFiberOrNot, EventFreeOnWakeup) {
            // This UT detects a use-after-free race, but it's can only be revealed by
            // UBSan in most cases, unfortunately.
            RunInFiber(100, GetParam(), [&](auto index) {
                for (int i = 0; i != 1000; ++i) {
                    auto ev = std::make_unique<wait_event>();
                    std::thread([&] { ev->set(); }).detach();
                    ev->wait();
                    ev = nullptr;
                }
            });
        }

        INSTANTIATE_TEST_SUITE_P(waitable, SystemFiberOrNot,
                                 ::testing::Values(true, false));

    }
}  // namespace abel
