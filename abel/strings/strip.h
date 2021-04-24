// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_STRINGS_STRIP_H_
#define ABEL_STRINGS_STRIP_H_

#include <cstddef>
#include <string>
#include "abel/base/profile.h"
#include "abel/strings/ascii.h"
#include "abel/strings/ends_with.h"
#include "abel/strings/starts_with.h"
#include <string_view>

namespace abel {


//  consume_prefix()
//
// Strips the `expected` prefix from the start of the given string, returning
// `true` if the strip operation succeeded or false otherwise.
//
// Example:
//
//   std::string_view input("abc");
//   EXPECT_TRUE(abel:: consume_prefix(&input, "a"));
//   EXPECT_EQ(input, "bc");
ABEL_FORCE_INLINE bool consume_prefix(std::string_view *str, std::string_view expected) {
    if (!abel::starts_with(*str, expected)) return false;
    str->remove_prefix(expected.size());
    return true;
}
//  consume_suffix()
//
// Strips the `expected` suffix from the end of the given string, returning
// `true` if the strip operation succeeded or false otherwise.
//
// Example:
//
//   std::string_view input("abcdef");
//   EXPECT_TRUE(abel:: consume_suffix(&input, "def"));
//   EXPECT_EQ(input, "abc");
ABEL_FORCE_INLINE bool consume_suffix(std::string_view *str, std::string_view expected) {
    if (!abel::ends_with(*str, expected)) return false;
    str->remove_suffix(expected.size());
    return true;
}

//  strip_prefix()
//
// Returns a view into the input string 'str' with the given 'prefix' removed,
// but leaving the original string intact. If the prefix does not match at the
// start of the string, returns the original string instead.
ABEL_MUST_USE_RESULT ABEL_FORCE_INLINE std::string_view strip_prefix(
        std::string_view str, std::string_view prefix) {
    if (abel::starts_with(str, prefix)) str.remove_prefix(prefix.size());
    return str;
}

//  strip_suffix()
//
// Returns a view into the input string 'str' with the given 'suffix' removed,
// but leaving the original string intact. If the suffix does not match at the
// end of the string, returns the original string instead.
ABEL_MUST_USE_RESULT ABEL_FORCE_INLINE std::string_view strip_suffix(
        std::string_view str, std::string_view suffix) {
    if (abel::ends_with(str, suffix)) str.remove_suffix(suffix.size());
    return str;
}


}  // namespace abel

#endif  // ABEL_STRINGS_STRIP_H_
