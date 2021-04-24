// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_STRING_END_WITH_H_
#define ABEL_STRING_END_WITH_H_

#include <string_view>
#include <string>
#include "abel/base/profile.h"

namespace abel {

/**
 * @brief Checks if the given suffix string is located at the end of this string.
 * @param text todo
 * @param suffix todo
 * @return todo
 */
ABEL_FORCE_INLINE bool ends_with(std::string_view text, std::string_view suffix) {
    return suffix.empty() ||
           (text.size() >= suffix.size() &&
            memcmp(text.data() + (text.size() - suffix.size()), suffix.data(),
                   suffix.size()) == 0);
}


/**
 * @brief Checks if the given suffix string is located at the end of this
 * string. Compares the characters case-insensitively.
 * @param text todo
 * @param suffix todo
 * @return todo
 */
bool ends_with_case(std::string_view text, std::string_view suffix);

}  // namespace abel

#endif  // ABEL_STRING_END_WITH_H_
