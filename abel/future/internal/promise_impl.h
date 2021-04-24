//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_FUTURE_INTERNAL_PROMISE_IMPL_H_
#define ABEL_FUTURE_INTERNAL_PROMISE_IMPL_H_


#include "abel/future/internal//promise.h"

#include <memory>
#include <utility>

#include "abel/future/internal/executor.h"
#include "abel/future/internal/future.h"

namespace abel {

    namespace future_internal {

        template<class... Ts>
        promise<Ts...>::promise()
                : core_(std::make_shared<future_core<Ts...>>(get_default_executor())) {}

        template<class... Ts>
        future<Ts...> promise<Ts...>::get_future() {
            return future<Ts...>(core_);
        }

        template<class... Ts>
        template<class... Us, class>
        void promise<Ts...>::set_value(Us &&... values) {
            core_->set_boxed(boxed<Ts...>(box_values, std::forward<Us>(values)...));
        }

        template<class... Ts>
        void promise<Ts...>::set_boxed(boxed<Ts...> boxed) {
            core_->set_boxed(std::move(boxed));
        }

        template<class... Ts>
        promise<Ts...>::promise(executor executor)
                : core_(std::make_shared<future_core<Ts...>>(std::move(executor))) {}

    }  // namespace future_internal
}  // namespace abel

#endif  // ABEL_FUTURE_INTERNAL_PROMISE_IMPL_H_
