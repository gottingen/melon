//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_ASIN_H_
#define ABEL_MATH_ASIN_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/atan.h>
#include <abel/math/sqrt.h>
#include <abel/math/abs.h>

namespace abel {


    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T asin_compute(const T x) ABEL_NOEXCEPT {
            return (x > T(1) ? std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::epsilon() > abs(x - T(1)) ?
                                                                     T(ABEL_HALF_PI) :
                                                                     std::numeric_limits<T>::epsilon() > abs(x) ?
                                                                     T(0) : atan(x / sqrt(T(1) - x * x))
            );
        }

        template<typename T>
        ABEL_CONSTEXPR T asin_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : x < T(0) ?
                                                                      -asin_compute(-x) : asin_compute(x)
            );
        }

    } //namespace math_internal

    template<typename T>
    ABEL_CONSTEXPR return_t<T> asin(const T x) ABEL_NOEXCEPT {
        return math_internal::asin_check(static_cast<return_t<T>>(x));
    }
} //namespace abel

#endif //ABEL_MATH_ASIN_H_
