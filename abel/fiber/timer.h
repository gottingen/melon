//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_TIMER_H_
#define ABEL_FIBER_TIMER_H_


#include <cstdint>
#include "abel/chrono/clock.h"
#include "abel/functional/function.h"

namespace abel {

    // Set a one-shot time.
    //
    // Timer ID returned by this method must be destroyed by `stop_timer`. This
    // method may only be called inside scheduling group.
    [[nodiscard]] std::uint64_t set_timer(abel::time_point at,
                                          abel::function<void()> &&cb);

    std::uint64_t set_timer(abel::time_point at,
                            abel::function<void(std::uint64_t)> &&cb);

    // Set a periodic timer.
    //
    // Timer ID returned by this method must be destroyed by `stop_timer`. This
    // method may only be called inside scheduling group.
    [[nodiscard]] std::uint64_t set_timer(abel::time_point at,
                                          abel::duration interval,
                                          abel::function<void()> &&cb);

    std::uint64_t set_timer(abel::time_point at,
                            abel::duration interval,
                            abel::function<void(std::uint64_t)> &&cb);

    // Set a periodic timer. `at` is assumed as `abel::time_now() +
    // internval`.
    //
    // Timer ID returned by this method must be destroyed by `stop_timer`. This
    // method may only be called inside scheduling group.
    [[nodiscard]] std::uint64_t set_timer(abel::duration interval,
                                          abel::function<void()> &&cb);

    std::uint64_t set_timer(abel::duration interval,
                            abel::function<void(std::uint64_t)> &&cb);

    // Detach `timer_id` without actually killing the timer.
    void detach_timer(std::uint64_t timer_id);

    // Shorthand for `detach_timer(set_timer(...))`.
    void set_detached_timer(abel::time_point at,
                            abel::function<void()> &&cb);

    void set_detached_timer(abel::time_point at,
                            abel::duration interval, abel::function<void()> &&cb);

    // Stop timer.
    //
    // You always need to call this unless the timer has been "detach"ed, otherwise
    // it's a leak.
    //
    // @sa: `timer_killer`.
    void stop_timer(std::uint64_t timer_id);

    // DEPRECATED for now. It's design is fundamentally broken. We should wait for
    // timer's fully termination in destructor of this class instead.
    //
    // This class kills timer on its destruction.
    //
    // It's hard to use correctly, though. If your timer callback is being called
    // concurrently, this class cannot help you to block until your callback
    // returns, since it have no idea about how to communicate with your callback.
    class timer_killer {
    public:
        timer_killer();

        explicit timer_killer(std::uint64_t timer_id);

        timer_killer(timer_killer &&tk) noexcept;

        timer_killer &operator=(timer_killer &&tk) noexcept;

        ~timer_killer();

        void reset(std::uint64_t timer_id = 0);

    private:
        std::uint64_t timer_id_ = 0;
    };

    [[nodiscard]] std::uint64_t create_worker_timer(
            abel::time_point at,
            abel::function<void(std::uint64_t)> &&cb);

    [[nodiscard]] std::uint64_t create_worker_timer(
            abel::time_point at, abel::duration interval,
            abel::function<void(std::uint64_t)> &&cb);

    // Enable timer previously created via `create_worker_timer`.
    void enable_worker_timer(std::uint64_t timer_id);

    void kill_worker_timer(std::uint64_t timer_id);


}  // namespace abel

#endif  // ABEL_FIBER_TIMER_H_
