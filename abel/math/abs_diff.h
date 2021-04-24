// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_MATH_ABS_DIFF_H_
#define ABEL_MATH_ABS_DIFF_H_


#include <vector>
#include <cstddef>
#include "abel/base/profile.h"

namespace abel {

/**
 * @brief todo
 * @tparam T todo
 * @param a todo
 * @param b todo
 * @return todo
 */
template<typename T>
ABEL_FORCE_INLINE T abs_diff(const T &a, const T &b) {
    return a > b ? a - b : b - a;
}

template<typename T>
ABEL_FORCE_INLINE T
sum_abs_diff(const std::vector<T> &X, const std::vector<T> &Y) {
    T val_out = T(0);
    size_t n_elem = X.size();

    for (size_t i = 0; i < n_elem; ++i) {
        val_out += abs(X[i] - Y[i]);
    }

    return val_out;
}

}  // namespace abel

#endif  // ABEL_MATH_ABS_DIFF_H_
