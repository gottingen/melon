//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_IS_ODD_H_
#define ABEL_MATH_IS_ODD_H_

#include <abel/base/profile.h>

namespace abel {


    ABEL_CONSTEXPR bool is_odd(const long long x) ABEL_NOEXCEPT {
        // return( x % long long(2) == long long(0) ? false : true );
        return (x & 1U) != 0;
    }

} //abel

#endif //ABEL_IS_ODD_H
