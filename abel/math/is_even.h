//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_IS_EVEN_H_
#define ABEL_MATH_IS_EVEN_H_

#include <abel/math/is_odd.h>

namespace abel {


    ABEL_CONSTEXPR bool is_even(const long long x) ABEL_NOEXCEPT {
        return !is_odd(x);
    }

} //namespace abel

#endif //ABEL_MATH_IS_EVEN_H_
