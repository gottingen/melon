//
// Created by liyinbin on 2019/12/8.
//

#include <abel/strings/ends_with.h>
#include <abel/strings/compare.h>
#include <abel/asl/ascii.h>
#include <algorithm>
#include <cstring>

namespace abel {


    bool ends_with_case(abel::string_view text, abel::string_view suffix) {
        if (text.size() >= suffix.size()) {
            return abel::compare_case(text.substr(text.size() - suffix.size()), suffix) == 0;
        }
        return false;
    }

} //namespace abel
