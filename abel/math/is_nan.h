//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_IS_NAN_H_
#define ABEL_MATH_IS_NAN_H_

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR bool is_nan(const T x) ABEL_NOEXCEPT {
            return x != x;
        }

        template<typename T1, typename T2>
        ABEL_CONSTEXPR bool any_nan(const T1 x, const T2 y) ABEL_NOEXCEPT {
            return (is_nan(x) || is_nan(y));
        }

        template<typename T1, typename T2>
        ABEL_CONSTEXPR bool all_nan(const T1 x, const T2 y) ABEL_NOEXCEPT {
            return (is_nan(x) && is_nan(y));
        }

        template<typename T1, typename T2, typename T3>
        ABEL_CONSTEXPR bool any_nan(const T1 x, const T2 y, const T3 z) ABEL_NOEXCEPT {
            return (is_nan(x) || is_nan(y) || is_nan(z));
        }

        template<typename T1, typename T2, typename T3>
        ABEL_CONSTEXPR bool all_nan(const T1 x, const T2 y, const T3 z) ABEL_NOEXCEPT {
            return (is_nan(x) && is_nan(y) && is_nan(z));
        }

    } //namespace math_internal
} //namespace abel

#endif //ABEL_MATH_IS_NAN_H_
