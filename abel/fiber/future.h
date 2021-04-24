//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_FUTURE_H_
#define ABEL_FIBER_FUTURE_H_


#include <utility>

#include "abel/memory/lazy_init.h"
#include "abel/future/future.h"
#include "abel/chrono/clock.h"
#include "abel/fiber/internal/waitable.h"

namespace abel {

    template <class... Ts>
    auto fiber_blocking_get(future<Ts...>&& f) {
        fiber_internal::wait_event ev;
        lazy_init<future_internal::boxed<Ts...>> receiver;

        // Once the `future` is satisfied, our continuation will move the
        // result into `receiver` and notify `cv` to wake us up.
        std::move(f).then([&](future_internal::boxed<Ts...> boxed) noexcept {
            // Do we need some synchronization primitives here to protect `receiver`?
            // `Event` itself guarantees `Set()` happens-before `Wait()` below, so
            // writing to `receiver` is guaranteed to happens-before reading it.
            //
            // But OTOH, what guarantees us that initialization of `receiver`
            // happens-before our assignment to it? `future::Then`?
            receiver.init(std::move(boxed));
            ev.set();
        });

        // Block until our continuation wakes us up.
        ev.wait();
        return std::move(*receiver).get();
    }

    // Same as `fiber_blocking_get` but this one accepts a timeout.
    template <class... Ts>
    auto fiber_blocking_try_get(future<Ts...>&& future,
                        const abel::time_point& timeout) {
        struct State {
            fiber_internal::oneshot_timed_event event;

            // Protects `receiver`.
            //
            // Unlike `fiber_blocking_get`, here it's possible that after `event.Wait()` times
            // out, concurrently the future is satisfied. In this case the continuation
            // of the future will be racing with us on `receiver`.
            std::mutex lock;
            std::optional<future_internal::boxed<Ts...>> receiver;

            explicit State(abel::time_point expires_at)
                    : event(expires_at) {}
        };
        auto state = std::make_shared<State>(timeout);

        // `state` must be copied here, in case of timeout, we'll leave the scope
        // before continuation is fired.
        std::move(future).then([state](future_internal::boxed<Ts...> boxed) noexcept {
            std::scoped_lock _(state->lock);
            state->receiver.emplace(std::move(boxed));
            state->event.set();
        });

        state->event.wait();
        std::scoped_lock _(state->lock);
        if constexpr (sizeof...(Ts) == 0) {
            return !!state->receiver;
        } else {
            return state->receiver ? std::optional(std::move(*state->receiver).get())
                                   : std::nullopt;
        }
    }

    template <class... Ts>
    auto fiber_blocking_try_get(future<Ts...>&& f,
        const abel::duration& timeout) {
        return fiber_blocking_try_get(std::move(f),abel::time_now() + timeout);
    }

    template <class... Ts>
    auto fiber_blocking_get(future<Ts...>* f) {
        return fiber_blocking_get(std::move(*f));
    }

    template <class... Ts>
    auto fiber_blocking_try_get(future<Ts...>* f,
                        const abel::time_point& timeout) {
        return fiber_blocking_try_get(std::move(*f), timeout);
    }

    template <class... Ts>
    auto fiber_blocking_try_get(future<Ts...>* f,
        const abel::duration& timeout) {
        return fiber_blocking_try_get(std::move(*f),abel::time_now() + timeout);
    }

}  // namespace abel

#endif  // ABEL_FIBER_FUTURE_H_
