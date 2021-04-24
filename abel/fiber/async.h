//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_ASYNC_H_
#define ABEL_FIBER_ASYNC_H_


#include <type_traits>
#include <utility>

#include "abel/future/future.h"
#include "abel/fiber/fiber_context.h"
#include "abel/fiber/fiber.h"

namespace abel {

    // Runs `f` with `args...` asynchronously.
    //
    // It's unspecified in which fiber (except the caller's own one) `f` is called.
    //
    // Note that this method is only available in fiber runtime. If you want to
    // start a fiber from pthread, use `start_fiber_from_pthread` (@sa: `fiber.h`)
    // instead.
    template <class F, class... Args,
            class R = future_internal::futurize_t<std::invoke_result_t<F&&, Args&&...>>>
    R fiber_async(fiber_internal::Launch policy, std::size_t scheduling_group,
            fiber_context* execution_context, F&& f, Args&&... args) {
        ABEL_ASSERT(policy == fiber_internal::Launch::Post || policy == fiber_internal::Launch::Dispatch);
        future_internal::as_promise_t<R> p;
        auto rc = p.get_future();

        // @sa: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0780r2.html
        auto proc = [p = std::move(p), f = std::forward<F>(f),
                args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            if constexpr (std::is_same_v<future<>, R>) {
                std::apply(f, std::move(args));
                p.set_value();
            } else {
                p.set_value(std::apply(f, std::move(args)));
            }
        };
        fiber_internal::start_fiber_detached(
                fiber::attributes{.launch_policy = policy,
                        .scheduling_group = scheduling_group,
                        .execution_context = execution_context},
                std::move(proc));
        return rc;
    }

    template <class F, class... Args,
            class R = future_internal::futurize_t<std::invoke_result_t<F&&, Args&&...>>>
    R fiber_async(fiber_internal::Launch policy, std::size_t scheduling_group, F&& f, Args&&... args) {
        return fiber_async(policy, scheduling_group, fiber_context::current(),
                     std::forward<F>(f), std::forward<Args>(args)...);
    }

    template <class F, class... Args,
            class R = future_internal::futurize_t<std::invoke_result_t<F&&, Args&&...>>>
    R fiber_async(fiber_internal::Launch policy, F&& f, Args&&... args) {
        return fiber_async(policy, fiber::kNearestSchedulingGroup, std::forward<F>(f),
                     std::forward<Args>(args)...);
    }

    template <class F, class... Args,
            class = std::enable_if_t<std::is_invocable_v<F&&, Args&&...>>>
    auto fiber_async(F&& f, Args&&... args) {
        return fiber_async(fiber_internal::Launch::Post, std::forward<F>(f), std::forward<Args>(args)...);
    }

}  // namespace  abel

#endif  // ABEL_FIBER_ASYNC_H_
