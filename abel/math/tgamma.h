//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_TGAMMA_H_
#define ABEL_MATH_TGAMMA_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math//find_whole.h>
#include <abel/math/abs.h>
#include <abel/math/exp.h>
#include <abel/math/log_gamma.h>
#include <limits>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T tgamma_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() :
                    std::numeric_limits<T>::epsilon() > abs(x - T(1))
                    ?
                    T(1) : std::numeric_limits<T>::epsilon() > abs(x)
                           ?
                           std::numeric_limits<T>::infinity() : x <
                                                                T(0) ?
                                                                std::numeric_limits<T>::epsilon() >
                                                                abs(x - find_whole(x))
                                                                ?
                                                                std::numeric_limits<T>::quiet_NaN()
                                                                :
                                                                tgamma_check(x + T(1)) / x
                                                                     : exp(lgamma(x)));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    ABEL_CONSTEXPR return_t<T> tgamma(const T x) ABEL_NOEXCEPT {
        return math_internal::tgamma_check(static_cast<return_t<T>>(x));
    }
}
#endif //ABEL_MATH_TGAMMA_H_
