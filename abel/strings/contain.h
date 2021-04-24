// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_BASE_STRING_CONTAIN_H_
#define ABEL_BASE_STRING_CONTAIN_H_

#include <string_view>
#include "abel/base/profile.h"

namespace abel {

//! Tests of string contains pattern

ABEL_FORCE_INLINE bool string_contains(std::string_view str, std::string_view match) {
    return str.find(match, 0) != str.npos;
}

}  // namespace abel

#endif  // ABEL_BASE_STRING_CONTAIN_H_
