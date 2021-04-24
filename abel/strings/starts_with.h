// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_BASE_STRING_START_WITH_H_
#define ABEL_BASE_STRING_START_WITH_H_

#include <string_view>
#include <string>
#include "abel/base/profile.h"

namespace abel {

/*!
 * Checks if the given match string is located at the start of this string.
 */
ABEL_FORCE_INLINE bool starts_with(std::string_view text, std::string_view prefix) {
    return prefix.empty() ||
           (text.size() >= prefix.size() &&
            memcmp(text.data(), prefix.data(), prefix.size()) == 0);
}

/*!
 * Checks if the given match string is located at the start of this
 * string. Compares the characters case-insensitively.
 */
bool starts_with_case(std::string_view text, std::string_view prefix);

}  // namespace abel

#endif  // ABEL_BASE_STRING_START_WITH_H_
