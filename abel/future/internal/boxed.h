//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_FUTURE_INTERNAL_BOXED_H_
#define ABEL_FUTURE_INTERNAL_BOXED_H_

#include <tuple>
#include <utility>
#include <variant>

#include "abel/future/internal/basics.h"
#include "abel/log/logging.h"

namespace abel {

    namespace future_internal {
        namespace detail {
            template<class T>
            T retrieve_boxed();  // T = boxed<...>.
        }  // namespace detail

        struct box_values_t {
            explicit box_values_t() = default;
        };

        inline constexpr box_values_t box_values;
        
        template<class... Ts>
        class boxed {
        public:
            static_assert(!types_contains_v<empty_type < Ts...>, void > );

            using value_type = std::tuple<Ts...>;

            // Constructs `boxed<...>` from values.
            template<class... Us, class = std::enable_if_t<
                    std::is_constructible_v<value_type, Us &&...>>>
            explicit boxed(box_values_t, Us &&... imms);

            // Conversion from compatible `boxed<...>`s.
            template<class... Us, class = std::enable_if_t<std::is_constructible_v<
                    value_type, std::tuple<Us &&...>>>>
            /* implicit */ boxed(boxed<Us...> boxed);

            // `get` returns different types in different cases due to implementation
            // limitations, unfortunately.
            //
            // - When `Ts...` is empty, `get` returns `void`;
            // - When there's exactly one type in `Ts...`, `get` returns (ref to) that
            //   type;
            // - Otherwise (there're at least two types in `Ts...`), `get` returns
            //   ref to `std::tuple<Ts...>`.
            //
            // There's a reason why we're specializing the case `sizeof...(Ts)` < 2:
            //
            // Once Coroutine TS is implemented, expression `co_await future;`
            // intuitively should be typed `void` or `T` in case there's none or one
            // type in `Ts...`, returning `std::tuple<>` or `std::tuple<T>` needlessly
            // complicated the user's life.
            //
            // (Indeed it's technically possible for us to specialize the type of the
            // `co_await` expression in `operator co_await` instead, but...)
            //
            // Another reason why we're specializing the case is that this is the
            // majority of use cases. Removing needless `std::tuple` ease our user
            // a lot even if it's an inconsistency.
            //
            // Also note that in case there're more than one types in `Ts`, the user
            // might leverage structured binding to ease their life with `auto&& [x, y]
            // = boxed.get();`
            //
            // Returns: See above.
            std::add_lvalue_reference_t<unboxed_type_t<Ts...>> get() &;

            std::add_rvalue_reference_t<unboxed_type_t<Ts...>> get() &&;

            // `get_raw` returns the `std::tuple` we're holding.
            value_type &get_raw() &;

            value_type &&get_raw() &&;

        private:
            template<class T>
            friend T detail::retrieve_boxed();

            template<class... Us>
            friend
            class boxed;

            // Indices in `holding_`.
            static constexpr std::size_t kEmpty = 0;
            static constexpr std::size_t kValue = 1;

            // A default constructed one is of little use except for being a placeholder.
            // None of the members (except for move assignment) may be called on a
            // default constructed `boxed<...>`.
            //
            // For internal use only.
            boxed() = default;

            std::variant<std::monostate, value_type> holding_;
        };

        static_assert(
                std::is_constructible_v<boxed<int, double>, box_values_t, int, char>);
        static_assert(
                std::is_constructible_v<boxed<int, double>, boxed<double, float>>);


        namespace detail {

            template<class T>
            T retrieve_boxed() {
                return T();
            }

        }  // namespace detail

        template<class... Ts>
        template<class... Us, class>
        boxed<Ts...>::boxed(box_values_t, Us &&... imms)
                : holding_(std::in_place_index<kValue>, std::forward<Us>(imms)...) {}

        template<class... Ts>
        template<class... Us, class>
        boxed<Ts...>::boxed(boxed<Us...> boxed) {
            holding_.template emplace<kValue>(
                    std::get<kValue>(std::move(boxed).holding_));
        }

        template<class... Ts>
        std::add_lvalue_reference_t<unboxed_type_t<Ts...>> boxed<Ts...>::get() &{
            DCHECK_NE(holding_.index(), kEmpty);
            if constexpr (sizeof...(Ts) == 0) {
                return (void) get_raw();
            } else if constexpr (sizeof...(Ts) == 1) {
                return std::get<0>(get_raw());
            } else {
                return get_raw();
            }
        }

        template<class... Ts>
        std::add_rvalue_reference_t<unboxed_type_t<Ts...>> boxed<Ts...>::get() &&{
            if constexpr (std::is_void_v<decltype(get())>) {
                return get();  // Nothing to move then.
            } else {
                return std::move(get());
            }
        }

        template<class... Ts>
        typename boxed<Ts...>::value_type &boxed<Ts...>::get_raw() &{
            DCHECK_NE(holding_.index(), kEmpty);
            return std::get<kValue>(holding_);
        }

        template<class... Ts>
        typename boxed<Ts...>::value_type &&boxed<Ts...>::get_raw() &&{
            return std::move(get_raw());
        }

    }  // namespace future_internal
}  // namespace abel

#endif  // ABEL_FUTURE_INTERNAL_BOXED_H_
