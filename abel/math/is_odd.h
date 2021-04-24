// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_MATH_IS_ODD_H_
#define ABEL_MATH_IS_ODD_H_

#include "abel/base/profile.h"

namespace abel {

ABEL_CONSTEXPR bool is_odd(const long long x) ABEL_NOEXCEPT {
    // return( x % long long(2) == long long(0) ? false : true );
    return (x & 1U) != 0;
}

}  // namespace abel

#endif  // ABEL_MATH_IS_ODD_H_
