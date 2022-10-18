
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_BASE_STRING_START_WITH_H_
#define MELON_BASE_STRING_START_WITH_H_

#include <string_view>
#include <string>
#include <cstring>
#include "melon/base/profile.h"

namespace melon {

/*!
 * Checks if the given match string is located at the start of this string.
 */
MELON_FORCE_INLINE bool starts_with(std::string_view text, std::string_view prefix) {
    return prefix.empty() ||
           (text.size() >= prefix.size() &&
            memcmp(text.data(), prefix.data(), prefix.size()) == 0);
}

/*!
 * Checks if the given match string is located at the start of this
 * string. Compares the characters case-insensitively.
 */
bool starts_with_case(std::string_view text, std::string_view prefix);

}  // namespace melon

#endif  // MELON_BASE_STRING_START_WITH_H_
