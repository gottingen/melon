
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "melon/strings/case_conv.h"

#include <algorithm>
#include "melon/strings/ascii.h"

namespace melon {

std::string &string_to_lower(std::string *str) {
    std::transform(str->begin(), str->end(), str->begin(),
                   [](char c) { return ascii::to_lower(c); });
    return *str;
}

std::string &string_to_upper(std::string *str) {
    std::transform(str->begin(), str->end(), str->begin(),
                   [](char c) { return ascii::to_upper(c); });
    return *str;
}

}
