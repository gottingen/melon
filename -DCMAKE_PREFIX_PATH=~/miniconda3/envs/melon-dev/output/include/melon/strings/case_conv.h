
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef MELON_STRINGS_CASE_CONV_H_
#define MELON_STRINGS_CASE_CONV_H_

#include <string>
#include <string_view>
#include "melon/base/profile.h"

namespace melon {

    std::string &string_to_lower(std::string *str);

    MELON_MUST_USE_RESULT MELON_FORCE_INLINE
    std::string string_to_lower(std::string_view str) {
        std::string result(str);
        melon::string_to_lower(&result);
        return result;
    }

    std::string &string_to_upper(std::string *str);

    MELON_MUST_USE_RESULT MELON_FORCE_INLINE
    std::string string_to_upper(std::string_view str) {
        std::string result(str);
        melon::string_to_upper(&result);
        return result;
    }

}
#endif  // MELON_STRINGS_CASE_CONV_H_
