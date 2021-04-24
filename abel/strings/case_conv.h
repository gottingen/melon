// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_STRINGS_CASE_CONV_H_
#define ABEL_STRINGS_CASE_CONV_H_

#include <string>
#include <string_view>
#include "abel/base/profile.h"

namespace abel {

std::string &string_to_lower(std::string *str);

ABEL_MUST_USE_RESULT ABEL_FORCE_INLINE
std::string string_to_lower(std::string_view str) {
    std::string result(str);
    abel::string_to_lower(&result);
    return result;
}

std::string &string_to_upper(std::string *str);

ABEL_MUST_USE_RESULT ABEL_FORCE_INLINE
std::string string_to_upper(std::string_view str) {
    std::string result(str);
    abel::string_to_upper(&result);
    return result;
}

}
#endif  // ABEL_STRINGS_CASE_CONV_H_
