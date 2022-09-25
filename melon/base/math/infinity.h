
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_BASE_MATH_INFINITY_H_
#define MELON_BASE_MATH_INFINITY_H_

#include <limits>
#include <cmath>
#include "melon/base/profile.h"

namespace melon::base {

    /// Can't use std::isinfinite() because it doesn't exist on windows.
    MELON_FORCE_INLINE bool is_finite(double d) {
        if (std::isnan(d)) return false;
        return d != std::numeric_limits<double>::infinity() &&
               d != -std::numeric_limits<double>::infinity();
    }

}  // namespace melon::base

#endif  // MELON_BASE_MATH_INFINITY_H_
