//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_SINH_H_
#define ABEL_MATH_SINH_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/abs.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T sinh_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::epsilon() > abs(x) ?
                                                                      T(0) : (exp(x) - exp(-x)) / T(2));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_CONSTEXPR return_t<T> sinh(const T x) ABEL_NOEXCEPT {
        return math_internal::sinh_check(static_cast<return_t<T>>(x));
    }

}
#endif //ABEL_MATH_SINH_H_
