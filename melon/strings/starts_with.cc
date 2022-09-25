
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "melon/strings/starts_with.h"
#include "melon/strings/compare.h"
#include <algorithm>

namespace melon {

bool starts_with_case(std::string_view text, std::string_view suffix) {
    if (text.size() >= suffix.size()) {
        return melon::compare_case(text.substr(0, suffix.size()), suffix) == 0;
    }
    return false;
}

}  // namespace melon
