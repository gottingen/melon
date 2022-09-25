
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef MELON_STRINGS_COMPARE_H_
#define MELON_STRINGS_COMPARE_H_

#include <string_view>
#include "melon/base/profile.h"

namespace melon {

    int compare_case(std::string_view a, std::string_view b);

    MELON_FORCE_INLINE bool equal_case(std::string_view a, std::string_view b) {
        return compare_case(a, b) == 0;
    }

}  // namespace melon

#endif  // MELON_STRINGS_COMPARE_H_
