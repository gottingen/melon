//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_CEIL_H_
#define ABEL_MATH_CEIL_H_

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR int ceil_resid(const T x, const T x_whole) ABEL_NOEXCEPT {
            return ((x > T(0)) && (x > x_whole));
        }

        template<typename T>
        ABEL_CONSTEXPR T ceil_int(const T x, const T x_whole) ABEL_NOEXCEPT {
            return (x_whole + static_cast<T>(ceil_resid(x, x_whole)));
        }

        template<typename T>
        ABEL_CONSTEXPR T ceil_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : !is_finite(x) ? x :
                                                                      std::numeric_limits<T>::epsilon() > abs(x) ? x :
                                                                      ceil_int(x, T(static_cast<long long>(x))));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_CONSTEXPR return_t<T> ceil(const T x) ABEL_NOEXCEPT {
        return math_internal::ceil_check(static_cast<return_t<T>>(x));
    }

} //namespace abel

#endif //ABEL_MATH_CEIL_H_
