//
// Created by liyinbin on 2019/12/8.
//

#ifndef ABEL_BASE_STRING_CONTAIN_H_
#define ABEL_BASE_STRING_CONTAIN_H_

#include <abel/strings/string_view.h>
#include <abel/base/profile.h>

namespace abel {

//! Tests of string contains pattern

    ABEL_FORCE_INLINE bool string_contains(abel::string_view str, abel::string_view match) {
        return str.find(match, 0) != str.npos;
    }

}
#endif //ABEL_BASE_STRING_CONTAIN_H_
