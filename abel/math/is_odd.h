//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_IS_ODD_H
#define ABEL_IS_ODD_H

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        ABEL_CONSTEXPR bool is_odd(const long long x) ABEL_NOEXCEPT {
            // return( x % long long(2) == long long(0) ? false : true );
            return (x & 1U) != 0;
        }

    } //namespace math_internal
} //abel

#endif //ABEL_IS_ODD_H
