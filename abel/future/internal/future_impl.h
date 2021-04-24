//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_FUTURE_INTERNAL_FUTURE_IMPL_H_
#define ABEL_FUTURE_INTERNAL_FUTURE_IMPL_H_


#include "abel/future/internal/future.h"

#include <memory>
#include <tuple>
#include <utility>

#include "abel/log/logging.h"

namespace abel {
    namespace future_internal {

        template<class... Ts>
        template<class... Us, class>
        future<Ts...>::future(futurize_values_t, Us &&... imms)
                : core_(std::make_shared<future_core<Ts...>>(get_default_executor())) {
            core_->set_boxed(boxed<Ts...>(box_values, std::forward<Us>(imms)...));
        }

        template<class... Ts>
        template<class... Us, class>
        future<Ts...>::future(futurize_tuple_t, std::tuple<Us...> values)
                : core_(std::make_shared<future_core<Ts...>>(get_default_executor())) {
                    core_->set_boxed(boxed<Ts...>(box_values, std::move(values)));
        }

        template<class... Ts>
        template<class U, class>
        future<Ts...>::future(U &&value)
                : core_(std::make_shared<future_core<Ts...>>(get_default_executor())) {
            static_assert(sizeof...(Ts) == 1);
            core_->set_boxed(boxed<Ts...>(box_values, std::forward<U>(value)));

        }

        template<class... Ts>
        template<class... Us, class>
        future<Ts...>::future(future<Us...> &&future) {
            promise<Ts...> p;

            // Here we "steal" `p.get_future()`'s core and "install" it into ourself,
            // thus once `p` is satisfied, we're satisfied as well.
            core_ = p.get_future().core_;
            std::move(future).then([p = std::move(p)](boxed<Us...> boxed) mutable {
                p.set_boxed(std::move(boxed));
            });
        }

        template<class... Ts>
        template<class F, class R>
        R future<Ts...>::then(F &&continuation) &&{
           DCHECK_MSG(core_ != nullptr,
                        "Calling `then` on uninitialized `future` is undefined.");

            // Evaluates to `true` if `F` can be called with `Ts...`.
            //
            // *It does NOT mean `F` cannot be called with `boxed<Ts>...` even if
            // this is true*, e.g.:
            //
            //    `f.then([] (auto&&...) {})`
            //
            // But it's technically impossible for us to tell whether the user is
            // expecting `Ts...` or `boxed<Ts>...` by checking `F`'s signature.
            //
            // In case there's an ambiguity, we prefer `Ts...`, and let the users do
            // disambiguation if this is not what they mean.
            constexpr auto kCallUnboxed = std::is_invocable_v<F, Ts...>;

            using NewType =
            typename std::conditional_t<kCallUnboxed, std::invoke_result<F, Ts...>,
                    std::invoke_result<F, boxed<Ts...>>>::type;
            using NewFuture = futurize_t<NewType>;
            using NewBoxed =
            std::conditional_t<std::is_void_v<NewType>, boxed<>, boxed<NewType>>;

            as_promise_t<NewFuture> p(core_->get_executor());  // Propagate the executor.
            auto result = p.get_future();

            auto raw_cont = [p = std::move(p), cont = std::forward<F>(continuation)](
                    boxed<Ts...> &&value) mutable noexcept {
                // The "boxed" value we get from calling `cont`.
                auto next_value = [&]() noexcept {
                    // Specializes the cases for `kCallUnboxed` being true / false.
                    auto next = [&] {
                        if constexpr (kCallUnboxed) {
                            return std::apply(cont, std::move(value.get_raw()));
                        } else {
                            return std::invoke(cont, std::move(value));
                        }
                    };
                    static_assert(std::is_same_v<NewType, decltype(next())>);

                    if constexpr (std::is_void_v<NewType>) {
                        next();
                        return NewBoxed(box_values);
                    } else {
                        return NewBoxed(box_values, next());
                    }
                }();

                // Unless we get a `future` from the continuation, we can now satisfy
                // the promise we made.
                if constexpr (!is_future_v<NewType>) {
                    p.set_boxed(std::move(next_value));
                } else {  // We do get a new `future`.
                    // Forward the value in the `future` we got to the promise we made.
                    next_value.get().core_->chain_action(
                            [p = std::move(p)](auto &&nested_v) mutable noexcept {
                                static_assert(
                                        std::is_same_v<as_boxed_t<NewFuture>,
                                                std::remove_reference_t<decltype(nested_v) >>);
                                p.set_boxed(std::move(nested_v));
                            });
                }
            };

            core_->chain_action(std::move(raw_cont));

            return result;
        }

    }  // namespace future_internal
}  // namespace abel

#endif  // ABEL_FUTURE_INTERNAL_FUTURE_IMPL_H_
