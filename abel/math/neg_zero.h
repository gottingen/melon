//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_NEG_ZERO_H_
#define ABEL_MATH_NEG_ZERO_H_

#include <abel/math/option.h>

namespace abel {

    namespace math_internal
    {

        template<typename T>
        ABEL_CONSTEXPR bool neg_zero(const T x) ABEL_NOEXCEPT {
            return( (x == T(0.0)) && (ABEL_COPYSIGN(T(1.0), x) == T(-1.0)) );
        }

    } //namespace math_internal

}
#endif //ABEL_MATH_NEG_ZERO_H_
