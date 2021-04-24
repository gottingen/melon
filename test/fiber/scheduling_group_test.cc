

#include "abel/fiber/internal/scheduling_group.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <thread>

#include "gflags/gflags.h"
#include "gtest/gtest.h"

#include "abel/base/random.h"
#include "abel/fiber/internal/fiber_entity.h"
#include "testing/fiber.h"
#include "abel/fiber/internal/timer_worker.h"
#include "abel/fiber/internal/waitable.h"

DECLARE_int32(fiber_run_queue_size);

using namespace std::literals;

namespace abel {
namespace fiber_internal {
    namespace {

        template<class SG, class... Args>
        [[nodiscard]] std::uint64_t SetTimerAt(SG &&sg, Args &&... args) {
            auto tid = sg->create_timer(std::forward<Args>(args)...);
            sg->enable_timer(tid);
            return tid;
        }

        std::size_t GetMaxFibers() {
            /*
            std::ifstream ifs("/proc/sys/vm/max_map_count");
            std::string s;
            std::getline(ifs, s);
            return std::min<std::size_t>(TryParse<std::size_t>(s).value_or(65530) / 2,
                                         1048576);
                                         */
            return 10000;
        }

    }  // namespace

    class SystemFiberOrNot : public ::testing::TestWithParam<bool> {
    };

    TEST(scheduling_group, Create) {
        // Hopefully we have at least 4 cores on machine running UT.
        auto sg =
                std::make_unique<scheduling_group>(core_affinity(), 20);

        // No affinity applied.
        auto sg2 =
                std::make_unique<scheduling_group>(core_affinity(), 20);
    }

    void WorkerTest(scheduling_group *sg, std::size_t index) {
        sg->enter_group(index);

        while (true) {
            fiber_entity *ready = sg->acquire_fiber();

            while (ready == nullptr) {
                ready = sg->wait_for_fiber();
                //DLOG_INFO("wait one");
            }
            if (ready == scheduling_group::kSchedulingGroupShuttingDown) {
                break;
            }
            ready->resume();
            ASSERT_EQ(get_current_fiber_entity(), get_master_fiber_entity());
        }
        sg->leave_group();
    }

    struct Context {
        std::atomic<std::size_t> executed_fibers{0};
        std::atomic<std::size_t> yields{0};
    } context;

    void FiberProc(Context *ctx) {
        auto sg = scheduling_group::current();
        auto self = get_current_fiber_entity();
        std::atomic<std::size_t> yield_count_local{0};

        // It can't be.
        ASSERT_NE(self, get_master_fiber_entity());
        for (int i = 0; i != 10; ++i) {
            ASSERT_EQ(fiber_state::Running, self->state);
            sg->yield(self);
            ++ctx->yields;
            ASSERT_EQ(self, get_current_fiber_entity());
            ++yield_count_local;
        }

        // The fiber is resumed by two worker concurrently otherwise.
        ASSERT_EQ(10, yield_count_local);

        ++ctx->executed_fibers;
    }

    TEST_P(SystemFiberOrNot, RunFibers) {
        context.executed_fibers = 0;
        context.yields = 0;

        FLAGS_fiber_run_queue_size = 262144 ;

        static const auto N = GetMaxFibers();
        DLOG_INFO("Starting {} fibers.", N);

        auto sg =
                std::make_unique<scheduling_group>(core_affinity(), 16);
        std::thread workers[16];
        timer_worker dummy(sg.get());
        sg->set_timer_worker(&dummy);

        for (int i = 0; i != 16; ++i) {
            workers[i] = std::thread(WorkerTest, sg.get(), i);
        }

        for (int i = 0; i != N; ++i) {
            // Even if we never call `FreeFiberEntity`, no leak should be reported (it's
            // implicitly recycled when `FiberProc` returns).
            testing::StartFiberEntityInGroup(sg.get(), GetParam(),
                                             [&] { FiberProc(&context); });
        }
        while (context.executed_fibers != N) {
            std::this_thread::sleep_for(100ms);
        }
        sg->stop();
        for (auto &&t : workers) {
            t.join();
        }
        ASSERT_EQ(N, context.executed_fibers);
        ASSERT_EQ(N * 10, context.yields);
    }


    std::atomic<std::size_t> switched{};

    void SwitchToNewFiber(scheduling_group *sg, bool system_fiber,
                          std::size_t left) {
        if (--left) {
            auto next = create_fiber_entity(sg, system_fiber, [sg, system_fiber, left] {
                SwitchToNewFiber(sg, system_fiber, left);
            });
            sg->switch_to(get_current_fiber_entity(), next);
        }
        ++switched;
    }

    TEST_P(SystemFiberOrNot, SwitchToFiber) {
        switched = 0;

        constexpr auto N = 16384;

        auto sg =
                std::make_unique<scheduling_group>(core_affinity(), 16);
        std::thread workers[16];
        timer_worker dummy(sg.get());
        sg->set_timer_worker(&dummy);

        for (int i = 0; i != 16; ++i) {
            workers[i] = std::thread(WorkerTest, sg.get(), i);
        }

        testing::StartFiberEntityInGroup(sg.get(), GetParam(), [&] {
            SwitchToNewFiber(sg.get(), GetParam(), N);
        });
        while (switched != N) {
            std::this_thread::sleep_for(100ms);
        }
        sg->stop();
        for (auto &&t : workers) {
            t.join();
        }
        ASSERT_EQ(N, switched);
    }

    TEST_P(SystemFiberOrNot, WaitForFiberExit) {

        FLAGS_fiber_run_queue_size = 262144;

        auto sg =
                std::make_unique<scheduling_group>(core_affinity(), 16);
        std::thread workers[16];
        timer_worker timer_worker(sg.get());
        sg->set_timer_worker(&timer_worker);

        for (int i = 0; i != 16; ++i) {
            workers[i] = std::thread(WorkerTest, sg.get(), i);
        }
        timer_worker.start();

        for (int k = 0; k != 100; ++k) {
            constexpr auto N = 1024;
            std::atomic<std::size_t> waited{};
            for (int i = 0; i != N; ++i) {
                auto f1 = create_fiber_entity(
                        sg.get(), Random() % 2 == 0 , [] {
                            waitable_timer wt(abel::time_now() + Random(10) * abel::duration::milliseconds(1));
                            wt.wait();
                        });
                f1->exit_barrier = get_ref_counted<exit_barrier>();
                auto f2 = create_fiber_entity(sg.get(), GetParam(),
                                            [&, wc = f1->exit_barrier] {
                                                wc->wait();
                                                ++waited;
                                            });
                if (Random() % 2 == 0) {
                    sg->ready_fiber(f1, {});
                    sg->ready_fiber(f2, {});
                } else {
                    sg->ready_fiber(f2, {});
                    sg->ready_fiber(f1, {});
                }
            }
            while (waited != N) {
                //std::cout<<waited<<std::endl;
                std::this_thread::sleep_for(10ms);
            }
        }
        sg->stop();
        timer_worker.stop();
        timer_worker.join();
        for (auto &&t : workers) {
            t.join();
        }

    }

    void SleepyFiberProc(std::atomic<std::size_t> *leaving) {
        auto self = get_current_fiber_entity();
        auto sg = self->scheduling_group;
        std::unique_lock lk(self->scheduler_lock);

        auto timer_cb = [self](auto) {
            std::unique_lock lk(self->scheduler_lock);
            self->state = fiber_state::Ready;
            self->scheduling_group->ready_fiber(self, std::move(lk));
        };
        auto timer_id =
                SetTimerAt(sg, abel::time_now() + abel::duration::seconds(1) + Random(1000000) * abel::duration::microseconds(1), timer_cb);

        sg->halt(self, std::move(lk));
        sg->remove_timer(timer_id);
        ++*leaving;
    }

    TEST_P(SystemFiberOrNot, SetTimer) {
        auto sg =
                std::make_unique<scheduling_group>(core_affinity(), 16);
        std::thread workers[16];
        std::atomic<std::size_t> leaving{0};
        timer_worker timer_worker(sg.get());
        sg->set_timer_worker(&timer_worker);

        for (int i = 0; i != 16; ++i) {
            workers[i] = std::thread(WorkerTest, sg.get(), i);
        }
        timer_worker.start();

        constexpr auto N = 30000;
        for (int i = 0; i != N; ++i) {
            testing::StartFiberEntityInGroup(sg.get(), GetParam(),
                                             [&] { SleepyFiberProc(&leaving); });
        }
        while (leaving != N) {
            std::this_thread::sleep_for(100ms);
        }
        sg->stop();
        timer_worker.stop();
        timer_worker.join();
        for (auto &&t : workers) {
            t.join();
        }
        ASSERT_EQ(N, leaving);
    }

    TEST_P(SystemFiberOrNot, SetTimerPeriodic) {
        auto sg =
                std::make_unique<scheduling_group>(core_affinity(), 1);
        timer_worker timer_worker(sg.get());
        sg->set_timer_worker(&timer_worker);
        auto worker = std::thread(WorkerTest, sg.get(), 0);
        timer_worker.start();

        auto start = abel::time_now();
        std::atomic<std::size_t> called{};
        std::atomic<std::uint64_t> timer_id;
        testing::StartFiberEntityInGroup(sg.get(), GetParam(), [&] {
            auto cb = [&](auto) {
                if (called < 10) {
                    ++called;
                }
            };
            timer_id =
                    SetTimerAt(sg, abel::time_now() + abel::duration::milliseconds(10), abel::duration::milliseconds(100), cb);
        });
        while (called != 10) {
            std::this_thread::sleep_for(1ms);
        }
        ASSERT_NEAR((abel::time_now() - start) / abel::duration::milliseconds(1),
                (abel::duration::milliseconds(20) + abel::duration::milliseconds(100) * 9) / abel::duration::milliseconds(1), 10);
        sg->remove_timer(timer_id);
        sg->stop();
        timer_worker.stop();
        timer_worker.join();
        worker.join();
        ASSERT_EQ(10, called);
    }

    INSTANTIATE_TEST_SUITE_P(scheduling_group, SystemFiberOrNot,
                             ::testing::Values(true, false));
}  // namespace fiber_internal
}  // namespace abel
