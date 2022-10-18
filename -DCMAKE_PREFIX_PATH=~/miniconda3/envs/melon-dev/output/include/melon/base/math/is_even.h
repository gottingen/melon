
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_BASE_MATH_IS_EVEN_H_
#define MELON_BASE_MATH_IS_EVEN_H_

#include "melon/base/math/is_odd.h"

namespace melon::base {


constexpr bool is_even(const long long x) noexcept {
    return !is_odd(x);
}

}  // namespace melon::base

#endif  // MELON_BASE_MATH_IS_EVEN_H_
