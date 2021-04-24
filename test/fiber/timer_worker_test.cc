
#include "abel/fiber/internal/timer_worker.h"

#include <memory>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "abel/thread/latch.h"
#include "abel/base/random.h"
#include "abel/fiber/internal/scheduling_group.h"

using namespace std::literals;

namespace abel {
    namespace fiber_internal {

        namespace {

            template<class SG, class... Args>
            [[nodiscard]] std::uint64_t set_timer_at(SG &&sg, Args &&... args) {
                auto tid = sg->create_timer(std::forward<Args>(args)...);
                sg->enable_timer(tid);
                return tid;
            }

        }  // namespace

        TEST(timer_worker, EarlyTimer) {
            std::atomic<bool> called = false;

            auto sg =
                    std::make_unique<scheduling_group>(core_affinity(), 1);
            timer_worker worker(sg.get());
            sg->set_timer_worker(&worker);
            std::thread t = std::thread([&sg, &called] {
                sg->enter_group(0);

                (void) set_timer_at(sg,
                                  abel::time_point::infinite_past(),
                                  [&](auto tid) {
                                      sg->remove_timer(tid);
                                      called = true;
                                  });
                std::this_thread::sleep_for(1s);
                sg->leave_group();
            });

            worker.start();
            t.join();
            worker.stop();
            worker.join();

            ASSERT_TRUE(called);
        }

        TEST(timer_worker, SetTimerInTimerContext) {
            std::atomic<bool> called = false;

            auto sg =
                    std::make_unique<scheduling_group>(core_affinity(), 1);
            timer_worker worker(sg.get());
            sg->set_timer_worker(&worker);
            std::thread t = std::thread([&sg, &called] {
                sg->enter_group(0);

                auto timer_cb = [&](auto tid) {
                    auto timer_cb2 = [&, tid](auto tid2) {
                        sg->remove_timer(tid);
                        sg->remove_timer(tid2);
                        called = true;
                    };
                    (void) set_timer_at(sg,
                                      abel::time_point{}, timer_cb2);
                };
                (void) set_timer_at(sg,
                                  abel::time_point::infinite_past(), timer_cb);
                std::this_thread::sleep_for(1s);
                sg->leave_group();
            });

            worker.start();
            t.join();
            worker.stop();
            worker.join();

            ASSERT_TRUE(called);
        }

        std::atomic<std::size_t> timer_set, timer_removed;

        TEST(timer_worker, Torture) {
            constexpr auto N = 100000;
            constexpr auto T = 40;

            auto sg =
                    std::make_unique<scheduling_group>(core_affinity(), T);
            timer_worker worker(sg.get());
            sg->set_timer_worker(&worker);
            std::thread ts[T];

            for (int i = 0; i != T; ++i) {
                ts[i] = std::thread([i, &sg] {
                    sg->enter_group(i);

                    for (int j = 0; j != N; ++j) {
                        auto timeout = abel::time_now() + abel::duration::milliseconds(Random(2000));
                        if (j % 2 == 0) {
                            // In this case we set a timer and let it fire.
                            (void) set_timer_at(sg, timeout,
                                              [&sg](auto timer_id) {
                                                  sg->remove_timer(timer_id);
                                                  ++timer_removed;
                                              });  // Indirectly calls `timer_worker::AddTimer`.
                            ++timer_set;
                        } else {
                            // In this case we set a timer and cancel it sometime later.
                            auto timer_id = set_timer_at(sg, timeout, [](auto) {});
                            (void) set_timer_at(sg,
                                              abel::time_now() +abel::duration::milliseconds(Random(1000)),
                                              [timer_id, &sg](auto self) {
                                                  sg->remove_timer(timer_id);
                                                  sg->remove_timer(self);
                                                  ++timer_removed;
                                              });
                            ++timer_set;
                        }
                        if (j % 10000 == 0) {
                            std::this_thread::sleep_for(100ms);
                        }
                    }

                    // Wait until all timers have been consumed. Otherwise if we leave the
                    // thread too early, `timer_worker` might incurs use-after-free when
                    // accessing our thread-local timer queue.
                    while (timer_removed.load(std::memory_order_relaxed) !=
                           timer_set.load(std::memory_order_relaxed)) {
                        std::this_thread::sleep_for(100ms);
                    }
                    sg->leave_group();
                });
            }

            worker.start();

            for (auto &&t : ts) {
                t.join();
            }
            worker.stop();
            worker.join();

            ASSERT_EQ(timer_set, timer_removed);
            ASSERT_EQ(N * T, timer_set);
        }
    }  // namespace fiber_internal
}  // namespace abel
