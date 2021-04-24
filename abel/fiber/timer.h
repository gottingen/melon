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
    // Timer ID returned by this method must be destroyed by `KillTimer`. This
    // method may only be called inside scheduling group.
    [[nodiscard]] std::uint64_t SetTimer(abel::time_point at,
                                         abel::function<void()>&& cb);
    std::uint64_t SetTimer(abel::time_point at,
                           abel::function<void(std::uint64_t)>&& cb);

    // Set a periodic timer.
    //
    // Timer ID returned by this method must be destroyed by `KillTimer`. This
    // method may only be called inside scheduling group.
    [[nodiscard]] std::uint64_t SetTimer(abel::time_point at,
                                         abel::duration interval,
                                         abel::function<void()>&& cb);
    std::uint64_t SetTimer(abel::time_point at,
                           abel::duration interval,
                           abel::function<void(std::uint64_t)>&& cb);

    // Set a periodic timer. `at` is assumed as `abel::time_now() +
    // internval`.
    //
    // Timer ID returned by this method must be destroyed by `KillTimer`. This
    // method may only be called inside scheduling group.
    [[nodiscard]] std::uint64_t SetTimer(abel::duration interval,
                                         abel::function<void()>&& cb);
    std::uint64_t SetTimer(abel::duration interval,
                           abel::function<void(std::uint64_t)>&& cb);

// Detach `timer_id` without actually killing the timer.
    void detach_timer(std::uint64_t timer_id);

// Shorthand for `detach_timer(SetTimer(...))`.
    void SetDetachedTimer(abel::time_point at,
                          abel::function<void()>&& cb);
    void SetDetachedTimer(abel::time_point at,
                          abel::duration interval, abel::function<void()>&& cb);

// Stop timer.
//
// You always need to call this unless the timer has been "detach"ed, otherwise
// it's a leak.
//
// @sa: `TimerKiller`.
    void KillTimer(std::uint64_t timer_id);

// DEPRECATED for now. It's design is fundamentally broken. We should wait for
// timer's fully termination in destructor of this class instead.
//
// This class kills timer on its destruction.
//
// It's hard to use correctly, though. If your timer callback is being called
// concurrently, this class cannot help you to block until your callback
// returns, since it have no idea about how to communicate with your callback.
    class TimerKiller {
    public:
        TimerKiller();
        explicit TimerKiller(std::uint64_t timer_id);
        TimerKiller(TimerKiller&& tk) noexcept;
        TimerKiller& operator=(TimerKiller&& tk) noexcept;
        ~TimerKiller();

        void Reset(std::uint64_t timer_id = 0);

    private:
        std::uint64_t timer_id_ = 0;
    };

// For internal use only. DO NOT USE IT.
//
// Timer callback for timers set by this method is called in timer worker's
// context. This can slow other timers down. Be careful about this.
//
// Execution context is NOT propagated by these internal methods.
    namespace fiber_internal {

// Two-stage timer creation.
//
// In certain case, you may want to store timer ID somewhere and access that ID
// in timer callback. Without this two-stage procedure, you need to synchronizes
// between timer-id-filling and timer-callback.
//
// Timer ID returned must be detached or killed. Otherwise a leak will occur.
        [[nodiscard]] std::uint64_t create_timer(
                std::chrono::steady_clock::time_point at,
                abel::function<void(std::uint64_t)>&& cb);
        [[nodiscard]] std::uint64_t create_timer(
                std::chrono::steady_clock::time_point at, std::chrono::nanoseconds interval,
                abel::function<void(std::uint64_t)>&& cb);

// Enable timer previously created via `create_timer`.
        void enable_timer(std::uint64_t timer_id);

// Kill the timer previously set.
        void KillTimer(std::uint64_t timer_id);

    }  // namespace fiber_internal

}  // namespace abel

#endif  // ABEL_FIBER_TIMER_H_
