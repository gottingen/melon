
#ifndef ABEL_FORMAT_INTERNAL_EXTENSION_H_
#define ABEL_FORMAT_INTERNAL_EXTENSION_H_

#include <climits>
#include <cstddef>
#include <cstring>
#include <ostream>
#include <abel/base/profile.h>
#include <abel/format/internal/output.h>
#include <abel/format/internal/format_flags.h>
#include <abel/format/internal/length_mod.h>
#include <abel/format/internal/conversion_char.h>
#include <abel/strings/string_view.h>

namespace abel {

class Cord;

namespace format_internal {

constexpr uint64_t ConversionCharToConvValue (char conv) {
    return
#define CONV_SET_CASE(c) \
  conv == #c[0] ? (uint64_t{1} << (1 + conversion_char::Id::c)):
        ABEL_CONVERSION_CHARS_EXPAND_(CONV_SET_CASE,)
        #undef CONV_SET_CASE
            conv == '*'
        ? 1
        : 0;
}

enum class Conv : uint64_t {
#define CONV_SET_CASE(c) c = ConversionCharToConvValue(#c[0]),
    ABEL_CONVERSION_CHARS_EXPAND_(CONV_SET_CASE,)
#undef CONV_SET_CASE

    // Used for width/precision '*' specification.
        star = ConversionCharToConvValue('*'),

    // Some predefined values:
        integral = d | i | u | o | x | X,
    floating = a | e | f | g | A | E | F | G,
    numeric = integral | floating,
    string = s,
    pointer = p
};

// Type safe OR operator.
// We need this for two reasons:
//  1. operator| on enums makes them decay to integers and the result is an
//     integer. We need the result to stay as an enum.
//  2. We use "enum class" which would not work even if we accepted the decay.
constexpr Conv operator | (Conv a, Conv b) {
    return Conv(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

// Get a conversion with a single character in it.
constexpr Conv ConversionCharToConv (char c) {
    return Conv(ConversionCharToConvValue(c));
}

// Checks whether `c` exists in `set`.
constexpr bool Contains (Conv set, char c) {
    return (static_cast<uint64_t>(set) & ConversionCharToConvValue(c)) != 0;
}

// Checks whether all the characters in `c` are contained in `set`
constexpr bool Contains (Conv set, Conv c) {
    return (static_cast<uint64_t>(set) & static_cast<uint64_t>(c)) ==
        static_cast<uint64_t>(c);
}

// Return type of the AbelFormatConvert() functions.
// The Conv template parameter is used to inform the framework of what
// conversion characters are supported by that AbelFormatConvert routine.
template<Conv C>
struct ConvertResult {
    static constexpr Conv kConv = C;
    bool value;
};
template<Conv C>
constexpr Conv ConvertResult<C>::kConv;



}  // namespace format_internal


}  // namespace abel

#endif  // ABEL_STRINGS_INTERNAL_STR_FORMAT_EXTENSION_H_
