//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_FUTURE_INTERNAL_BASICS_H_
#define ABEL_FUTURE_INTERNAL_BASICS_H_

#include "abel/meta/type_traits.h"

namespace abel {
    namespace future_internal {

        // The helpers for easier use..
        template<class... Ts>
        constexpr auto are_rvalue_refs_v =
                std::conjunction_v < std::is_rvalue_reference < Ts >
        ...>;
        static_assert(are_rvalue_refs_v<int &&, char &&>);
        static_assert(!are_rvalue_refs_v<int, char &&>);

        // Rebinds `Ts...` in `TT<...>` to `UT<...>`.
        //
        // rebind_t<future<int, double>, promise<>> -> promise<int, double>.
        template<class T, template<class...> class UT>
        struct rebind;
        template<template<class...> class TT, class... Ts,
                template<class...> class UT>
        struct rebind<TT<Ts...>, UT> {
            using type = UT<Ts...>;
        };

        template<class T, template<class...> class UT>
        using rebind_t = typename rebind<T, UT>::type;
        static_assert(std::is_same_v < rebind_t<std::common_type<>, empty_type>, empty_type<>>);
        static_assert(std::is_same_v < rebind_t<std::common_type < int, char>, empty_type > ,
                empty_type < int, char >> );

        // Basic templates.
        template<class... Ts>
        class future;

        template<class... Ts>
        class promise;

        template<class... Ts>
        class boxed;

        // Test if a given type `T` is a `future<...>`.
        template<class T>
        struct is_future : std::false_type {
        };
        template<class... Ts>
        struct is_future<future<Ts...>> : std::true_type {
        };

        template<class T>
        constexpr auto is_future_v = is_future<T>::value;
        static_assert(!is_future_v<char *>);
        static_assert(is_future_v<future<>>);
        static_assert(is_future_v<future<char *>>);
        static_assert(is_future_v<future<char *, int>>);

        template<class... Ts>
        constexpr auto is_futures_v = std::conjunction_v<is_future<Ts>...>;

        // "Futurize" a type.
        template<class... Ts>
        struct futurize {
            using type = future<Ts...>;
        };
        template<>
        struct futurize<void> : futurize<> {
        };
        template<class... Ts>
        struct futurize<future<Ts...>> : futurize<Ts...> {
        };

        template<class... Ts>
        using futurize_t = typename futurize<Ts...>::type;
        static_assert(std::is_same_v < futurize_t<int>, future<int>>);
        static_assert(std::is_same_v < futurize_t<future<>>, future<>>);
        static_assert(std::is_same_v < futurize_t<future<int>>, future<int>>);

        // Concatenate `Ts...` in multiple `future<Ts...>`s.
        //
        // flatten<future<T1, T2>, future<T3>> -> future<T1, T2, T3>.
        template<class... Ts>
        struct flatten {
            using type = rebind_t<types_cat_t < rebind_t<Ts, empty_type>...>, future>;
        };

        template<class... Ts>
        using flatten_t = typename flatten<Ts...>::type;
        static_assert(
                std::is_same_v < flatten_t<future<void *>, future<>, future<char>>,
                future<void *, char>>);  // Empty parameter pack is ignored.
        static_assert(std::is_same_v < flatten_t<future<>, future<>>, future<>>);

        // Shortcuts for `rebind_t<..., boxed>`, `rebind_t<..., promise>`.
        //
        // Used primarily for rebinding `future<Ts...>`.
        template<class T>
        using as_boxed_t = rebind_t<T, boxed>;
        template<class T>
        using as_promise_t = rebind_t<T, promise>;
        static_assert(std::is_same_v < as_boxed_t<future<>>, boxed<>>);
        static_assert(std::is_same_v < as_boxed_t<future<int, char>>, boxed<int, char>>);
        static_assert(std::is_same_v < as_promise_t<future<>>, promise<>>);
        static_assert(
                std::is_same_v < as_promise_t<future<int, char>>, promise<int, char>>);

        // Value type we'll get from `boxed<...>::get`. Also used in several other
        // places.
        //
        // Depending on what's in `Ts...`, `unboxed_type_t<Ts...>` can be:
        //
        // - `void` if `Ts...` is empty;
        // - `T` is there's only one type `T` in `Ts...`.;
        // - `std::tuple<Ts...>` otherwise.
        template<class... Ts>
        using unboxed_type_t = typename std::conditional_t<
                sizeof...(Ts) == 0, std::common_type < void>,
        std::conditional_t<sizeof...(Ts) == 1, std::common_type<Ts...>,
                std::common_type<std::tuple < Ts...>>>>::type;
    }  // namespace future_internal
}  // namespace abel

#endif  // ABEL_FUTURE_INTERNAL_BASICS_H_
