
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef MELON_STRINGS_CHAR_CONV_H_
#define MELON_STRINGS_CHAR_CONV_H_

#include <system_error>
#include "melon/base/profile.h"

namespace melon {

enum class chars_format {
    scientific = 1,
    fixed = 2,
    hex = 4,
    general = fixed | scientific,
};

struct from_chars_result {
    const char *ptr;
    std::errc ec;
};

melon::from_chars_result from_chars(const char *first, const char *last,
                                   double &value,  // NOLINT
                                   chars_format fmt = chars_format::general);

melon::from_chars_result from_chars(const char *first, const char *last,
                                   float &value,  // NOLINT
                                   chars_format fmt = chars_format::general);

// std::chars_format is specified as a bitmask type, which means the following
// operations must be provided:
MELON_FORCE_INLINE constexpr chars_format operator&(chars_format lhs, chars_format rhs) {
    return static_cast<chars_format>(static_cast<int>(lhs) &
                                     static_cast<int>(rhs));
}

MELON_FORCE_INLINE constexpr chars_format operator|(chars_format lhs, chars_format rhs) {
    return static_cast<chars_format>(static_cast<int>(lhs) |
                                     static_cast<int>(rhs));
}

MELON_FORCE_INLINE constexpr chars_format operator^(chars_format lhs, chars_format rhs) {
    return static_cast<chars_format>(static_cast<int>(lhs) ^
                                     static_cast<int>(rhs));
}

MELON_FORCE_INLINE constexpr chars_format operator~(chars_format arg) {
    return static_cast<chars_format>(~static_cast<int>(arg));
}

MELON_FORCE_INLINE chars_format &operator&=(chars_format &lhs, chars_format rhs) {
    lhs = lhs & rhs;
    return lhs;
}

MELON_FORCE_INLINE chars_format &operator|=(chars_format &lhs, chars_format rhs) {
    lhs = lhs | rhs;
    return lhs;
}

MELON_FORCE_INLINE chars_format &operator^=(chars_format &lhs, chars_format rhs) {
    lhs = lhs ^ rhs;
    return lhs;
}

}  // namespace melon

#endif  // MELON_STRINGS_CHAR_CONV_H_
