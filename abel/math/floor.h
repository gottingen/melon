//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_FLOOR_H_
#define ABEL_MATH_FLOOR_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/is_finit.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR int floor_resid(const T x, const T x_whole) ABEL_NOEXCEPT {
            return ((x < T(0)) && (x < x_whole));
        }

        template<typename T>
        ABEL_CONSTEXPR T floor_int(const T x, const T x_whole) ABEL_NOEXCEPT {
            return (x_whole - static_cast<T>(floor_resid(x, x_whole)));
        }

        template<typename T>
        ABEL_CONSTEXPR T floor_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ?
                    std::numeric_limits<T>::quiet_NaN() :
                    !is_finite(x) ? x :
                    std::numeric_limits<T>::epsilon() > abs(x) ?
                    x : floor_int(x, T(static_cast<long long>(x))));
        }

    } //namespace abel


    template<typename T>
    ABEL_CONSTEXPR return_t<T> floor(const T x) ABEL_NOEXCEPT {
        return math_internal::floor_check(static_cast<return_t<T>>(x));
    }
}
#endif //ABEL_MATH_FLOOR_H_
