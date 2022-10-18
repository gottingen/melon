
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_BASE_STRING_CONTAIN_H_
#define MELON_BASE_STRING_CONTAIN_H_

#include <string_view>
#include "melon/base/profile.h"

namespace melon {

//! Tests of string contains pattern

MELON_FORCE_INLINE bool string_contains(std::string_view str, std::string_view match) {
    return str.find(match, 0) != str.npos;
}

}  // namespace melon

#endif  // MELON_BASE_STRING_CONTAIN_H_
