//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_MANTISSA_H_
#define ABEL_MATH_MANTISSA_H_

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T mantissa(const T x) ABEL_NOEXCEPT {
            return (x < T(1) ? mantissa(x * T(10)) : x > T(10) ? mantissa(x / T(10)) : x);
        }

    } //namespace math_internal

}
#endif //ABEL_MATH_MANTISSA_H_