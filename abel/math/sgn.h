// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_MATH_SGN_H_
#define ABEL_MATH_SGN_H_

namespace abel {


template<typename T>
ABEL_CONSTEXPR int sgn(const T x) ABEL_NOEXCEPT {
    return (x > T(0) ? 1 : x < T(0) ? -1 : 0);
}
}  // namespace abel

#endif  // ABEL_MATH_SGN_H_
