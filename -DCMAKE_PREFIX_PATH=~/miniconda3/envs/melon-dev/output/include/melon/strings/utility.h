
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_STRINGS_UTILITY_H_
#define MELON_STRINGS_UTILITY_H_
#include <string_view>
#include <string>
#include "melon/base/profile.h"

namespace melon {

    inline char front_char(const std::string_view &s) { return s[0]; }

    inline char back_char(const std::string_view &s) { return s[s.size() - 1]; }

    MELON_MUST_USE_RESULT inline std::string as_string(const std::string_view &str) {
        return std::string(str.data(), str.size());
    }

    inline void copy_to_string(const std::string_view &str, std::string *out) {
        out->assign(str.data(), str.size());
    }

    inline void append_to_string(const std::string_view &str, std::string *out) {
        out->append(str.data(), str.size());
    }

}  // namespace melon

#endif // MELON_STRINGS_UTILITY_H_
