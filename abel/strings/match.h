//
//
// -----------------------------------------------------------------------------
// File: match.h
// -----------------------------------------------------------------------------
//
// This file contains simple utilities for performing string matching checks.
// All of these function parameters are specified as `abel::string_view`,
// meaning that these functions can accept `std::string`, `abel::string_view` or
// NUL-terminated C-style strings.
//
// Examples:
//   std::string s = "foo";
//   abel::string_view sv = "f";
//   assert(abel::string_contains(s, sv));
//
// Note: The order of parameters in these functions is designed to mimic the
// order an equivalent member function would exhibit;
// e.g. `s.Contains(x)` ==> `abel::string_contains(s, x).
#ifndef ABEL_STRINGS_MATCH_H_
#define ABEL_STRINGS_MATCH_H_

#include <cstring>

#include <abel/strings/string_view.h>

namespace abel {


// string_contains()
//
// Returns whether a given string `haystack` contains the substring `needle`.
ABEL_FORCE_INLINE bool string_contains(abel::string_view haystack, abel::string_view needle) {
  return haystack.find(needle, 0) != haystack.npos;
}

// starts_with()
//
// Returns whether a given string `text` begins with `prefix`.
ABEL_FORCE_INLINE bool starts_with(abel::string_view text, abel::string_view prefix) {
  return prefix.empty() ||
         (text.size() >= prefix.size() &&
          memcmp(text.data(), prefix.data(), prefix.size()) == 0);
}

// ends_with()
//
// Returns whether a given string `text` ends with `suffix`.
ABEL_FORCE_INLINE bool ends_with(abel::string_view text, abel::string_view suffix) {
  return suffix.empty() ||
         (text.size() >= suffix.size() &&
          memcmp(text.data() + (text.size() - suffix.size()), suffix.data(),
                 suffix.size()) == 0);
}

// EqualsIgnoreCase()
//
// Returns whether given ASCII strings `piece1` and `piece2` are equal, ignoring
// case in the comparison.
bool EqualsIgnoreCase(abel::string_view piece1, abel::string_view piece2);

// starts_with_case()
//
// Returns whether a given ASCII string `text` starts with `prefix`,
// ignoring case in the comparison.
bool starts_with_case(abel::string_view text, abel::string_view prefix);

// ends_with_case()
//
// Returns whether a given ASCII string `text` ends with `suffix`, ignoring
// case in the comparison.
bool ends_with_case(abel::string_view text, abel::string_view suffix);


}  // namespace abel

#endif  // ABEL_STRINGS_MATCH_H_
