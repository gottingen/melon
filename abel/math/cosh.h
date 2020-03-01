//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_COSH_H_
#define ABEL_MATH_COSH_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/abs.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T cosh_compute(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ?
                    std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::epsilon() > abs(x) ? T(1) :
                                                          (exp(x) + exp(-x)) / T(2));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_CONSTEXPR return_t<T> cosh(const T x) ABEL_NOEXCEPT {
        return math_internal::cosh_compute(static_cast<return_t<T>>(x));
    }

} //namespace abel

#endif //ABEL_MATH_COSH_H_
