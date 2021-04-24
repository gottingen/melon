//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_FUTURE_INTERNAL_PROMISE_H_
#define ABEL_FUTURE_INTERNAL_PROMISE_H_

#include <memory>
#include <tuple>

#include "abel/future/internal/basics.h"
#include "abel/future/internal/core.h"

namespace abel {

    namespace future_internal {

        template<class... Ts>
        class promise {
        public:
            using value_type = std::tuple<Ts...>;

            promise();

            // Non-copyable.
            promise(const promise &) = delete;

            promise &operator=(const promise &) = delete;

            // Movable.
            promise(promise &&) = default;

            promise &operator=(promise &&) = default;

            static_assert(!types_contains_v < empty_type < Ts...>, void>,
            "There's no point in specifying `void` in `Ts...`, use "
            "`promise<>` if you meant to declare a future with no value.");

            // Returns: `future` that is satisfied when one of `SetXxx` is called.
            //
            // May only be called once. (But how should we check this?)
            future<Ts...> get_future();

            // Satisfy the future with values.
            template<class... Us, class = std::enable_if_t<
                    std::is_constructible_v<value_type, Us &&...>>>
            void set_value(Us &&... values);

            // Satisfy the future with a boxed value.
            void set_boxed(boxed<Ts...> boxed);

        private:
            template<class... Us>
            friend
            class future;

            // Construct a `promise` with `executor` instead of what
            // `get_default_executor()` gives.
            //
            // There's not much harm if we expose this constructor to the users, but
            // I'm not sure if we want to give the user the ability to choose a different
            // executor for each `promise` they make.
            explicit promise(executor executor);

            std::shared_ptr<future_core<Ts...>> core_;
        };

    }  // namespace future_internal
}  // namespace abel

#endif  // ABEL_FUTURE_INTERNAL_PROMISE_H_
