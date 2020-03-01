//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_STRING_START_WITH_H_
#define ABEL_BASE_STRING_START_WITH_H_

#include <abel/strings/string_view.h>
#include <string>

namespace abel {

/*!
 * Checks if the given match string is located at the start of this string.
 */
    ABEL_FORCE_INLINE bool starts_with(abel::string_view text, abel::string_view prefix) {
        return prefix.empty() ||
               (text.size() >= prefix.size() &&
                memcmp(text.data(), prefix.data(), prefix.size()) == 0);
    }

/*!
 * Checks if the given match string is located at the start of this
 * string. Compares the characters case-insensitively.
 */
    bool starts_with_case(abel::string_view text, abel::string_view prefix);

}
#endif //ABEL_BASE_STRING_START_WITH_H_
