//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_EXPM1_H
#define ABEL_EXPM1_H

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/abs.h>
#include <abel/math/exp.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T expm1_compute(const T x) ABEL_NOEXCEPT {
            // return x * ( T(1) + x * ( T(1)/T(2) + x * ( T(1)/T(6) + x * ( T(1)/T(24) +  x/T(120) ) ) ) ); // O(x^6)
            return x + x * (x / T(2) + x * (x / T(6) + x * (x / T(24) + x * x / T(120)))); // O(x^6)
        }

        template<typename T>
        ABEL_CONSTEXPR T expm1_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : abs(x) > T(1e-04) ?
                                                                      exp(x) - T(1) : expm1_compute(x));
        }

    } // namespace math_internal

    template<typename T>
    ABEL_CONSTEXPR return_t<T> expm1(const T x) ABEL_NOEXCEPT {
        return math_internal::expm1_check(static_cast<return_t<T>>(x));
    }


}
#endif //ABEL_EXPM1_H
