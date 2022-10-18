
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_BASE_MATH_IS_ODD_H_
#define MELON_BASE_MATH_IS_ODD_H_

namespace melon::base {

    constexpr bool is_odd(const long long x) noexcept {
        return (x & 1U) != 0;
    }

}  // namespace melon::base

#endif  // MELON_BASE_MATH_IS_ODD_H_
