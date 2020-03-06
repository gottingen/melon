//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_ASINH_H_
#define ABEL_MATH_ASINH_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/log.h>
#include <abel/math/sqrt.h>
#include <abel/math/abs.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T asinh_compute(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ? std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::epsilon() > abs(x) ?
                                                                      T(0) : log(x + sqrt(x * x + T(1)))
            );
        }

    }

    template<typename T>
    ABEL_DEPRECATED_MESSAGE("use std version instead")
    ABEL_CONSTEXPR return_t<T> asinh(const T x) ABEL_NOEXCEPT {
        return math_internal::asinh_compute(static_cast<return_t<T>>(x));
    }

}
#endif //ABEL_MATH_ASINH_H_
