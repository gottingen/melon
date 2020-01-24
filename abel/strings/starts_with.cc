//
// Created by liyinbin on 2019/12/8.
//
#include <abel/strings/starts_with.h>
#include <abel/strings/compare.h>
#include <algorithm>

namespace abel {

bool starts_with_case (abel::string_view text, abel::string_view suffix) {
    if (text.size() >= suffix.size()) {
        return abel::compare_case(text.substr(0, suffix.size()), suffix) == 0;
    }
    return false;
}

}

