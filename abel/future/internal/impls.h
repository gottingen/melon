//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_FUTURE_INTERNAL_IMPLS_H_
#define ABEL_FUTURE_INTERNAL_IMPLS_H_

#include <type_traits>
#include <utility>

namespace abel {

    namespace future_internal {
        namespace detail {

            template<std::size_t I, class F>
            void for_each_indexed_impl(F &&) {}  // Nothing to do.

            template<std::size_t I, class F, class T, class... Ts>
            void for_each_indexed_impl(F &&f, T &&t, Ts &&... ts) {
                f(std::forward<T>(t), std::integral_constant<std::size_t, I>());

                if constexpr (sizeof...(Ts) != 0) {
                    for_each_indexed_impl<I + 1>(std::forward<F>(f), std::forward<Ts>(ts)...);
                }
            }

            template<template<class...> class TT, class T>
            struct is_template_t : std::false_type {
            };

            template<template<class...> class TT, class... T>
            struct is_template_t<TT, TT<T...>> : std::true_type {
            };

            template<class T>
            constexpr auto is_duration_v = is_template_t<std::chrono::duration, T>::value;
            template<class T>
            constexpr auto is_time_point_v =
                    is_template_t<std::chrono::time_point, T>::value;

            static_assert(is_duration_v<std::chrono::steady_clock::duration>);
            static_assert(!is_duration_v<std::chrono::steady_clock::time_point>);
            static_assert(!is_time_point_v<std::chrono::steady_clock::duration>);
            static_assert(is_time_point_v<std::chrono::steady_clock::time_point>);

            // This traits convert `T` to `std::optional<T>` if `T` is NOT `void`, otherwise
            // `bool` is returned.
            template<class T>
            struct optional_or_bool {
                using type = std::optional<T>;
            };
            template<>
            struct optional_or_bool<void> {
                using type = bool;
            };

            template<class T>
            using optional_or_bool_t = typename optional_or_bool<T>::type;

        }  // namespace detail

        template<class F, class... Ts>
        void for_each_indexed(F &&f, Ts &&... ts) {
            detail::for_each_indexed_impl<0>(std::forward<F>(f), std::forward<Ts>(ts)...);
        }

    }  // namespace future_internal
}  // namespace abel

#endif  // ABEL_FUTURE_INTERNAL_IMPLS_H_
