//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_IS_FINIT_H
#define ABEL_IS_FINIT_H

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/is_inf.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR bool is_finite(const T x) ABEL_NOEXCEPT {
            return (!is_nan(x)) && (!is_inf(x));
        }

        template<typename T1, typename T2>
        ABEL_CONSTEXPR bool any_finite(const T1 x, const T2 y) ABEL_NOEXCEPT {
            return (is_finite(x) || is_finite(y));
        }

        template<typename T1, typename T2>
        ABEL_CONSTEXPR bool all_finite(const T1 x, const T2 y) ABEL_NOEXCEPT {
            return (is_finite(x) && is_finite(y));
        }

        template<typename T1, typename T2, typename T3>
        ABEL_CONSTEXPR bool any_finite(const T1 x, const T2 y, const T3 z) ABEL_NOEXCEPT {
            return (is_finite(x) || is_finite(y) || is_finite(z));
        }

        template<typename T1, typename T2, typename T3>
        ABEL_CONSTEXPR bool all_finite(const T1 x, const T2 y, const T3 z) ABEL_NOEXCEPT {
            return (is_finite(x) && is_finite(y) && is_finite(z));
        }

    }
}
#endif //ABEL_IS_FINIT_H
