//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_FUTURE_INTERNAL_FUTURE_H_
#define ABEL_FUTURE_INTERNAL_FUTURE_H_


#include <memory>
#include <tuple>
#include <utility>

#include "abel/future/internal/basics.h"
#include "abel/future/internal/boxed.h"
#include "abel/future/internal/core.h"

namespace abel {

    namespace future_internal {
        // The names duplicate what `box_values_t` / ... serve for..
        //
        // We could have chosen `from_values_t` / ... to unify the names, but the
        // corresponding variable `from_values` / ... is likely to clash with some
        // methods' name..
        struct futurize_values_t {
            explicit constexpr futurize_values_t() = default;
        };  // @sa: LWG 2510
        struct futurize_tuple_t {
            explicit constexpr futurize_tuple_t() = default;
        };

        inline constexpr futurize_values_t futurize_values;
        inline constexpr futurize_tuple_t futurize_tuple;

        template<class... Ts>
        class future {
        public:
            // Construct a "ready" future from immediate values.
            //
            // This overload does not participate in overload resolution unless
            // `std::tuple<Ts...>` (or `boxed<Ts...>`, to be more specifically) is
            // constructible from `Us&&...`.
            template<class... Us, class = std::enable_if_t < std::is_constructible_v <
                                          boxed < Ts...>, box_values_t, Us&&...>>>

            explicit future(futurize_values_t, Us &&... imms);

            // Construct a "ready" future from `std::tuple<Us...>` as long as
            // `boxed<Ts...>` is constructible from `boxed<Us...>`.
            //
            // We're accepting values (rather than refs) here, performance may hurt
            // if something like `std::array<int, 12345678>` is listed in `Ts...`.
            template<class... Us, class = std::enable_if_t <std::is_constructible_v<
                    boxed < Ts...>, boxed<Us...>>>>
            future(futurize_tuple_t,
            std::tuple<Us...> values
            );

            // Construct a "ready" future from an immediate value. Shortcut for
            // `future(futurize_values, value)`.
            //
            // Only participate in overload resolution when:
            //
            // - `sizeof...(Ts)` is 1;
            // - `T` (The only type in `Ts...`) is constructible from `U`.
            // - `U` is not `future<...>`.
            template<class U, class = std::enable_if_t<
                    std::is_constructible_v <
                    typename std::conditional_t<
                            sizeof...(Ts) == 1, std::common_type < Ts...>,
                    std::common_type < void>> ::type,
            U> &&
            !is_future_v <U>>>

            /* implicit */ future(U &&value);

            // Conversion between compatible types is allowed.
            //
            // future: Its value (once satisfied) is used to satisfy the `future` being
            //         constructed. Itself is invalidated on return.
            template<class... Us, class = std::enable_if_t <std::is_constructible_v<
                    boxed < Ts...>, boxed<Us...>>>>
            /* implicit */ future(
            future<Us...> &&future
            );  // Rvalue-ref intended.

            // Default constructor constructs an empty future which is not of much use
            // except for being a placeholder.
            //
            // It might be easier for our user to use if the default constructor instead
            // constructs a ready future if `Ts...` is empty, though. Currently they're
            // forced to write `future(futurize_values)`.
            //
            //   If we do this, we can allow `future(1, 2.0, "3")` to construct a ready
            //   future of type `future<int, double, const char *>` as well. This really
            //   is a nice feature to have.
            //
            //   The reason we currently don't do this is that it's just inconsistent
            //   with the cases when `Ts...` is not empty, unless we also initialize a
            //   ready future in those cases. But if we do initialize the `future` that
            //   way, the constructor can be too heavy to justify the convenience this
            //   brings. Besides, this will require `Ts...` to be DefaultConstructible,
            //   which currently we do not require.
            future() = default;

            // Non-copyable.
            //
            // DECLARE_UNCOPYABLE / Inheriting from `Uncopyable` prevents the compiler
            // from generating move constructor / operator, leaving us to defaulting
            // those members.
            //
            // But using the macro for uncopyable while rolling our own for moveable
            // makes the code harder to reason about, so we explicitly delete the copy
            // constructor / operator here.
            future(const future &) = delete;

            future &operator=(const future &) = delete;

            // Movable.
            future(future &&) = default;

            future &operator=(future &&) = default;

            // It's meaningless to use `void` in `Ts...`, simply use `future<>` is
            // enough.
            //
            // Let's raise a hard error for `void` then.
            static_assert(!types_contains_v < empty_type < Ts...>, void>,
            "There's no point in specifying `void` in `Ts...`, use "
            "`future<>` if you meant to declare a future with no value.");

            // Given that:
            //
            // - We support chaining continuation;
            // - The value satisfied us is / will be *moved* into continuation's
            //   argument;
            //
            // It's fundamentally broken to provide a `Get` on `future` as we have no
            // way to get the value back from the continuation's argument.
            //
            // `get` and the continuation will race, as well.
            [
            [deprecated("`future` does not support blocking `Get`, use `blocking_get` "
            "instead.")]] void
            get();  // Not implemented.

            // `then` chains a continuation to `future`. The continuation is called
            // once the `future` is satisfied.
            //
            // `then<Executor, F>` might be implemented later if needed.
            //
            // continuation:
            //
            //   Either `continuation(Ts...)` or `continuation(boxed<Ts...>)` should
            //   be well-formed. When both are valid, the former is preferred.
            //
            //     Should we allow continuations accepting no parameters (meaning that
            //     the user is going to ignore the value) be passed in even if `Ts...`
            //     is not empty? I tried to write `future<Ts...>().then([&] { set_flag();
            //     });` multiple times when writing UT, and don't see any confusion
            //     there. I'm not sure if it's of much use in practice though..
            //
            //   Please note that when the parameter types of `continuation` are declared
            //   with `auto` (or `auto &&`, or anything alike), it's technically
            //   infeasible to check whether the continuation is excepting `Ts...` or
            //   `boxed<Ts...>`. In this case, the former is preferred.
            //
            //   If `boxed<Ts...>` is needed, specify the type in `continuation`'s
            //   signature explicitly.
            //
            // Returns:
            //
            //   - In case the `continuation` returns another `future`, a equivalent
            //     one is returned by `then`;
            //   - Otherwise (the `continuation` returns a immediate value) `then`
            //     returns a `future` that is satisfied with the value `continuation`
            //     returns once it's called.
            //
            // Note:
            //
            //   `then` is only allowed on rvalue-ref, use `std::move(future)` when
            //   needed.
            //
            //   Executor of result is inherited from *this. Even if the continuation
            //   returns a `future` using a different executor, only the value (but
            //   not the executor) is forwarded to the result.
            template<class F,
                    class R = futurize_t<typename std::conditional_t<
                            std::is_invocable_v < F, Ts...>, std::invoke_result<F, Ts...>,
                            std::invoke_result<F, boxed < Ts...>>> ::type>>

            R then(F &&continuation) &&;

            // In most cases, `then` is called on temporaries (which are rvalues)
            // rather than lvalues: `get_xxx_async().then(...).then(...)`, and this
            // overload won't be chosen.
            //
            // But in case it is chosen, we should prevent the code from compiling,
            // to force the user to mark the invalidation of the `future` explicitly
            // using `std::move`.
            template<class F>
            [
            [deprecated("`future` must be rvalue-qualified to call `then` on it. "
            "Use `std::move(future).then(...)` instead.")]] void
            then(F &&) &;  // Not implemented.

        private:
            friend class promise<Ts...>;

            template<class... Us>
            friend
            class future;

            // `promise` uses this constructor to build `future` in its `get_future`.
            explicit future(std::shared_ptr <future_core<Ts...>> core) : core_(std::move(core)) {}

            // Internal state shared with `promise<...>`.
            std::shared_ptr <future_core<Ts...>> core_;
        };


        template<class... Us>
        future(futurize_values_t, Us &&...) -> future<std::decay_t < Us>...>;
        template<class... Us>
        future(futurize_tuple_t, std::tuple<Us...>) -> future<std::decay_t < Us>...>;
        template<class U>
        future(U &&) -> future<std::decay_t < U>>;

    }  // namespace future_internal
}  //  namespace abel

#endif  // ABEL_FUTURE_INTERNAL_FUTURE_H_
