//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_IS_EVEN_H_
#define ABEL_MATH_IS_EVEN_H_

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        ABEL_CONSTEXPR bool is_even(const long long x) ABEL_NOEXCEPT {
            return !is_odd(x);
        }

    } //namespace math_internal

} //namespace abel

#endif //ABEL_MATH_IS_EVEN_H_
