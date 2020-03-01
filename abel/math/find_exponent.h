//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_FIND_EXPONENT_H
#define ABEL_FIND_EXPONENT_H

#include <abel/math/option.h>


namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR long long find_exponent(const T x, const long long exponent) ABEL_NOEXCEPT {
            return (x < T(1) ?
                    find_exponent(x * T(10), exponent - (long long)(1)) :
                    x > T(10) ?
                    find_exponent(x / T(10), exponent + (long long)(1)) :
                    exponent);
        }

    } //namespace math_internal

}
#endif //ABEL_FIND_EXPONENT_H
