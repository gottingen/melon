
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_DETAIL_UTILITY_H_
#define MELON_FUTURE_DETAIL_UTILITY_H_


#include <optional>
#include "melon/future/expected.h"

namespace melon {

    template<typename Alloc, typename... Ts>
    class basic_future;

    // Determines wether a type is a Future<...>
    template<typename T>
    struct is_future : public std::false_type {
    };

    template<typename Alloc, typename... Ts>
    struct is_future<basic_future<Alloc, Ts...>> : public std::true_type {
    };

    template<typename T>
    constexpr bool is_future_v = is_future<T>::value;
}  // namespace melon

namespace melon::future_internal {

    // Determines wether a type is a expected<...>
    template<typename T>
    struct is_expected : public std::false_type {
    };

    template<typename T>
    struct is_expected<expected<T, std::exception_ptr>> : public std::true_type {
    };

    template<typename T>
    constexpr bool is_expected_v = is_expected<T>::value;

    // Function: get_first_error()
    // Returns the first error in a set of expected<>, if any
    template<typename... Ts>
    struct get_first_error_impl {
        static std::optional<std::exception_ptr> exec() { return std::nullopt; }
    };

    template<typename T, typename... Ts>
    struct get_first_error_impl<T, Ts...> {
        static std::optional<std::exception_ptr> exec(const melon::expected<T, std::exception_ptr> &first,
                                                      const expected <Ts, std::exception_ptr> &... rest) {
            if (!first.has_value()) {
                return first.error();
            }
            return get_first_error_impl<Ts...>::exec(rest...);
        }
    };

    template<typename... Ts>
    std::optional<std::exception_ptr> get_first_error(const expected <Ts, std::exception_ptr> &... vals) {
        return get_first_error_impl<Ts...>::exec(vals...);
    }

    // Converts a Future<T> into T
    template<typename T>
    struct decay_future {
        using type = T;
    };

    template<typename Alloc, typename T>
    struct decay_future<basic_future < Alloc, T>> {
    using type = T;
    };

    template<typename T>
    using decay_future_t = typename decay_future<T>::type;

    // Concatenates Two tuple types
    template<typename LhsT, typename RhsT>
    using tuple_cat_t =
    decltype(std::tuple_cat(std::declval<LhsT>(), std::declval<RhsT>()));

    template<typename... Ts>
    struct future_value_type;

    // Determines the full_fillment type of a Future<Ts...>
    template<typename... Ts>
    struct full_fill_type {
        using type = std::tuple<>;
    };

    template<typename... Ts>
    struct full_fill_type<void, Ts...> {
        using type = typename full_fill_type<Ts...>::type;
    };

    template<typename T, typename... Ts>
    struct full_fill_type<T, Ts...> {
        using lhs_t = std::tuple<T>;
        using rhs_t = typename full_fill_type<Ts...>::type;

        using type = tuple_cat_t<lhs_t, rhs_t>;
    };

    template<typename FTypeT>
    struct full_fill_to_value {
        using type = FTypeT;
    };

    template<>
    struct full_fill_to_value<std::tuple<>> {
        using type = void;
    };

    template<typename T>
    struct full_fill_to_value<std::tuple<T>> {
        using type = T;
    };

    // Determines the full_fillment type of a Future<Ts...>
    template<typename... Ts>
    using full_fill_type_t = typename full_fill_type<Ts...>::type;

    template<typename T, typename... Ts>
    struct future_value_type<T, Ts...> {
        using type = T;
    };

    template<typename T, typename U, typename... Ts>
    struct future_value_type<T, U, Ts...> {
        using f_type = full_fill_type_t<T, U, Ts...>;

        using type = typename full_fill_to_value<f_type>::type;
    };

    template<typename... Ts>
    using future_value_type_t = typename future_value_type<Ts...>::type;

    // Determines the finishing type of a Future<Ts...>
    template<typename... Ts>
    using finish_type_t = std::tuple<expected < Ts, std::exception_ptr>...>;

    // Determines the hard failure type of a Future<Ts...>
    template<typename... Ts>
    using fail_type_t = std::exception_ptr;

    template<std::size_t i, typename... Ts>
    auto finish_to_full_fill(std::tuple<Ts...> &&src) {
        (void) src;
        assert(std::get<i>(src).has_value());

        using val_t = typename std::tuple_element_t<i, std::tuple<Ts...>>::value_type;
        if constexpr (std::is_same_v<void, val_t>) {
            if constexpr (i == 0) {
                return std::tuple<>();
            } else {
                return finish_to_full_fill<i - 1>(std::move(src));
            }
        } else {
            auto val_tup = std::tuple<val_t>(std::move(*std::get<i>(src)));
            if constexpr (i == 0) {
                return val_tup;
            } else {
                return std::tuple_cat(finish_to_full_fill<i - 1>(std::move(src)),
                                      std::move(val_tup));
            }
        }
    }

    template<std::size_t i, std::size_t j, typename Result_t, typename... Ts>
    auto full_fill_to_finish(std::tuple<Ts...> &&src) {
        (void) src;
        using dst_t = std::tuple_element_t<i, Result_t>;
        if constexpr (std::is_same_v<expected < void, std::exception_ptr>, dst_t >) {
            if constexpr (i == std::tuple_size_v<Result_t> - 1) {
                return std::tuple<expected < void, std::exception_ptr>>
                ();
            } else {
                return std::tuple_cat(
                        std::tuple<expected < void, std::exception_ptr>>
                (),
                        full_fill_to_finish<i + 1, j, Result_t>(std::move(src)));
            }
        } else {
            using val_t = expected<std::tuple_element_t<j, std::tuple<Ts...>>, std::exception_ptr>;
            std::tuple<val_t> tup_val(std::move(std::get<j>(src)));

            if constexpr (i == std::tuple_size_v<Result_t> - 1) {
                return tup_val;
            } else {
                auto recur = full_fill_to_finish<i + 1, j + 1, Result_t>(std::move(src));
                return std::tuple_cat(tup_val, recur);
            }
        }
    }

    template<std::size_t i, typename T>
    auto fail_to_expect(const std::exception_ptr &src) {
        (void) src;
        using element_t = std::tuple_element_t<i, T>;
        std::tuple<element_t> val_tup(melon::unexpected_type<std::exception_ptr>{src});

        if constexpr (i >= std::tuple_size_v<T> - 1) {
            return val_tup;
        } else {
            return std::tuple_cat(val_tup, fail_to_expect<i + 1, T>(src));
        }
    }

    // A special Immediate queue tag type
    struct immediate_queue {
        template<typename F>
        static void push(F &&f) {
            f();
        }
    };

    inline void no_op_test() {}

    // Determines wether T's duck-typed push() method is static.
    // This will be used in Future_handler_base to omit its instantiation
    template<typename T, typename = void>
    struct has_static_push : std::false_type {
    };

    template<typename T>
    struct has_static_push<T, decltype(void(T::push(no_op_test)))>
            : std::true_type {
    };

    template<typename T>
    constexpr bool has_static_push_v = has_static_push<T>::value;

    // enqueue(), duck-type push f into q.
    // If Q has a static_push method, then q is ignored.
    template<typename Q, typename F>
    std::enable_if_t<!has_static_push_v<Q>> enqueue(Q *q, F &&f) {
        q->push(std::forward<F>(f));
    }

    template<typename Q, typename F>
    std::enable_if_t<has_static_push_v<Q>> enqueue(Q *, F &&f) {
        Q::push(std::forward<F>(f));
    }

}  // namespace melon::future_internal

#endif  // MELON_FUTURE_DETAIL_UTILITY_H_
