//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_SIN_H
#define ABEL_SIN_H

#include <abel/math/option.h>
#include <abel/math/is_nan.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T sin_compute(const T x) ABEL_NOEXCEPT {
            return T(2) * x / (T(1) + x * x);
        }

        template<typename T>
        ABEL_CONSTEXPR T sin_check(const T x) ABEL_NOEXCEPT {
            return (
                    is_nan(x) ?
                    std::numeric_limits<T>::quiet_NaN() :
                    std::numeric_limits<T>::epsilon() > abs(x) ?
                    T(0) :
                    std::numeric_limits<T>::epsilon() > abs(x - T(ABEL_HALF_PI)) ?
                    T(1) :
                    std::numeric_limits<T>::epsilon() > abs(x + T(ABEL_HALF_PI)) ?
                    -T(1) :
                    std::numeric_limits<T>::epsilon() > abs(x - T(ABEL_PI)) ?
                    T(0) :
                    std::numeric_limits<T>::epsilon() > abs(x + T(ABEL_PI)) ?
                    -T(0) :
                    sin_compute(tan(x / T(2))));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_CONSTEXPR return_t<T> sin(const T x) ABEL_NOEXCEPT {
        return math_internal::sin_check(static_cast<return_t<T>>(x));
    }
}
#endif //ABEL_SIN_H
