//
// Created by liyinbin on 2021/4/5.
//


#include "abel/fiber/timer.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <utility>

#include "abel/fiber/internal/scheduling_group.h"
#include "abel/fiber/fiber_context.h"
#include "abel/fiber/fiber.h"
#include "abel/fiber/runtime.h"

namespace abel {

    std::uint64_t set_timer(abel::time_point at,
                            abel::function<void()> &&cb) {
        return set_timer(at, [cb = std::move(cb)](auto) { cb(); });
    }

    std::uint64_t set_timer(abel::time_point at,
                            abel::function<void(std::uint64_t)> &&cb) {
        auto ec = ref_ptr(ref_ptr_v, fiber_context::current());
        auto mcb = [cb = std::move(cb), ec = std::move(ec)](auto timer_id) mutable {
            // Note that we're called in timer's worker thread, not in fiber
            // context. So fire a fiber to run user's code.
            fiber_internal::start_fiber_detached(
                    fiber::attributes{.execution_context = ec.get()},
                    [cb = std::move(cb), timer_id] { cb(timer_id); });
        };

        auto sg = fiber_internal::nearest_scheduling_group();
        auto timer_id = sg->create_timer(at, std::move(mcb));
        sg->enable_timer(timer_id);
        return timer_id;
    }

    std::uint64_t set_timer(abel::time_point at,
                            abel::duration interval,
                            abel::function<void()> &&cb) {
        return set_timer(at, interval, [cb = std::move(cb)](auto) { cb(); });
    }

    std::uint64_t set_timer(abel::time_point at,
                            abel::duration interval,
                            abel::function<void(std::uint64_t)> &&cb) {
        // This is ugly. But since we have to start a fiber each time user's `cb` is
        // called, we must share it.
        //
        // We also take measures not to call user's callback before the previous call
        // has returned. Otherwise we'll likely crash user's (presumably poor) code.
        struct UserCallback {
            void Run(std::uint64_t tid) {
                if (!running.exchange(true, std::memory_order_acq_rel)) {
                    cb(tid);
                }
                running.store(false, std::memory_order_relaxed);
                // Otherwise this call is lost. This can happen if user's code runs too
                // slowly. For the moment we left the behavior as unspecified.
            }

            abel::function<void(std::uint64_t)> cb;
            std::atomic<bool> running{};
        };

        auto ucb = std::make_shared<UserCallback>();
        auto ec = ref_ptr(ref_ptr_v, fiber_context::current());
        ucb->cb = std::move(cb);

        auto mcb = [ucb, ec = std::move(ec)](auto tid) mutable {
            fiber_internal::start_fiber_detached(
                    fiber::attributes{.execution_context = ec.get()},
                    [ucb, tid] { ucb->cb(tid); });
        };

        auto sg = fiber_internal::nearest_scheduling_group();
        auto timer_id = sg->create_timer(at, interval, std::move(mcb));
        sg->enable_timer(timer_id);
        return timer_id;
    }

    std::uint64_t set_timer(abel::duration interval,
                            abel::function<void()> &&cb) {
        return set_timer(abel::time_now() + interval, interval, std::move(cb));
    }

    std::uint64_t set_timer(abel::duration interval,
                            abel::function<void(std::uint64_t)> &&cb) {
        return set_timer(abel::time_now() + interval, interval, std::move(cb));
    }

    void detach_timer(std::uint64_t timer_id) {
        return fiber_internal::scheduling_group::get_timer_owner(timer_id)->detach_timer(
                timer_id);
    }

    void set_detached_timer(abel::time_point at,
                            abel::function<void()> &&cb) {
        detach_timer(set_timer(at, std::move(cb)));
    }

    void set_detached_timer(abel::time_point at,
                            abel::duration interval,
                            abel::function<void()> &&cb) {
        detach_timer(set_timer(at, interval, std::move(cb)));
    }

    void stop_timer(std::uint64_t timer_id) {
        return fiber_internal::scheduling_group::get_timer_owner(timer_id)->remove_timer(
                timer_id);
    }

    timer_killer::timer_killer() = default;

    timer_killer::timer_killer(std::uint64_t timer_id) : timer_id_(timer_id) {}

    timer_killer::timer_killer(timer_killer &&tk) noexcept : timer_id_(tk.timer_id_) {
        tk.timer_id_ = 0;
    }

    timer_killer &timer_killer::operator=(timer_killer &&tk) noexcept {
        reset();
        timer_id_ = std::exchange(tk.timer_id_, 0);
        return *this;
    }

    timer_killer::~timer_killer() { reset(); }

    void timer_killer::reset(std::uint64_t timer_id) {
        if (auto tid = std::exchange(timer_id_, 0)) {
            stop_timer(tid);
        }
        timer_id_ = timer_id;
    }


    [[nodiscard]] std::uint64_t create_worker_timer(
            abel::time_point at,
            abel::function<void(std::uint64_t)> &&cb) {
        return fiber_internal::nearest_scheduling_group()->create_timer(at, std::move(cb));
    }

    [[nodiscard]] std::uint64_t create_worker_timer(
            abel::time_point at, abel::duration interval,
            abel::function<void(std::uint64_t)> &&cb) {
        return fiber_internal::nearest_scheduling_group()->create_timer(at, interval,
                                                                        std::move(cb));
    }

    void enable_worker_timer(std::uint64_t timer_id) {
        fiber_internal::scheduling_group::get_timer_owner(timer_id)->enable_timer(timer_id);
    }

    void kill_worker_timer(std::uint64_t timer_id) {
        return fiber_internal::scheduling_group::get_timer_owner(timer_id)->remove_timer(
                timer_id);
    }

}  // namespace abel
