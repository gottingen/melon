// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_BASE_MATH_INFINITY_H_
#define ABEL_BASE_MATH_INFINITY_H_

#include <limits>
#include <cmath>
#include "abel/base/profile.h"

namespace abel {

/// Can't use std::isinfinite() because it doesn't exist on windows.
ABEL_FORCE_INLINE bool is_finite(double d) {
    if (std::isnan(d)) return false;
    return d != std::numeric_limits<double>::infinity() &&
           d != -std::numeric_limits<double>::infinity();
}

}  // namespace abel

#endif  // ABEL_BASE_MATH_INFINITY_H_
