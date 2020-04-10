//
//
// -----------------------------------------------------------------------------
// File: marshalling.h
// -----------------------------------------------------------------------------
//
// This header file defines the API for extending abel flag support to
// custom types, and defines the set of overloads for fundamental types.
//
// Out of the box, the abel flags library supports the following types:
//
// * `bool`
// * `int16_t`
// * `uint16_t`
// * `int32_t`
// * `uint32_t`
// * `int64_t`
// * `uint64_t`
// * `float`
// * `double`
// * `std::string`
// * `std::vector<std::string>`
// * `abel::LogSeverity` (provided here due to dependency ordering)
//
// Note that support for integral types is implemented using overloads for
// variable-width fundamental types (`short`, `int`, `long`, etc.). However,
// you should prefer the fixed-width integral types (`int32_t`, `uint64_t`,
// etc.) we've noted above within flag definitions.

//
// In addition, several abel libraries provide their own custom support for
// abel flags.
//
// The abel time library provides the following support for civil time values:
//
// * `abel::chrono_second`
// * `abel::chrono_minute`
// * `abel::chrono_hour`
// * `abel::chrono_day`
// * `abel::chrono_month`
// * `abel::chrono_year`
//
// and also provides support for the following absolute time values:
//
// * `abel::duration`
// * `abel::abel_time`
//
// Additional support for abel types will be noted here as it is added.
//
// You can also provide your own custom flags by adding overloads for
// `abel_parse_flag()` and `abel_unparse_flag()` to your type definitions. (See
// below.)
//
// -----------------------------------------------------------------------------
// Adding Type Support for abel Flags
// -----------------------------------------------------------------------------
//
// To add support for your user-defined type, add overloads of `abel_parse_flag()`
// and `abel_unparse_flag()` as free (non-member) functions to your type. If `T`
// is a class type, these functions can be friend function definitions. These
// overloads must be added to the same namespace where the type is defined, so
// that they can be discovered by Argument-Dependent Lookup (ADL).
//
// Example:
//
//   namespace foo {
//
//   enum OutputMode { kPlainText, kHtml };
//
//   // abel_parse_flag converts from a string to OutputMode.
//   // Must be in same namespace as OutputMode.
//
//   // Parses an OutputMode from the command line flag value `text. Returns
//   // `true` and sets `*mode` on success; returns `false` and sets `*error`
//   // on failure.
//   bool abel_parse_flag(abel::string_view text,
//                      OutputMode* mode,
//                      std::string* error) {
//     if (text == "plaintext") {
//       *mode = kPlainText;
//       return true;
//     }
//     if (text == "html") {
//       *mode = kHtml;
//      return true;
//     }
//     *error = "unknown value for enumeration";
//     return false;
//  }
//
//  // abel_unparse_flag converts from an OutputMode to a string.
//  // Must be in same namespace as OutputMode.
//
//  // Returns a textual flag value corresponding to the OutputMode `mode`.
//  std::string abel_unparse_flag(OutputMode mode) {
//    switch (mode) {
//      case kPlainText: return "plaintext";
//      case kHtml: return "html";
//    }
//    return abel::string_cat(mode);
//  }
//
// Notice that neither `abel_parse_flag()` nor `abel_unparse_flag()` are class
// members, but free functions. `abel_parse_flag/abel_unparse_flag()` overloads
// for a type should only be declared in the same file and namespace as said
// type. The proper `abel_parse_flag/abel_unparse_flag()` implementations for a
// given type will be discovered via Argument-Dependent Lookup (ADL).
//
// `abel_parse_flag()` may need, in turn, to parse simpler constituent types
// using `abel::parse_flag()`. For example, a custom struct `MyFlagType`
// consisting of a `std::pair<int, std::string>` would add an `abel_parse_flag()`
// overload for its `MyFlagType` like so:
//
// Example:
//
//   namespace my_flag_type {
//
//   struct MyFlagType {
//     std::pair<int, std::string> my_flag_data;
//   };
//
//   bool abel_parse_flag(abel::string_view text, MyFlagType* flag,
//                      std::string* err);
//
//   std::string abel_unparse_flag(const MyFlagType&);
//
//   // Within the implementation, `abel_parse_flag()` will, in turn invoke
//   // `abel::parse_flag()` on its constituent `int` and `std::string` types
//   // (which have built-in abel flag support.
//
//   bool abel_parse_flag(abel::string_view text, MyFlagType* flag,
//                      std::string* err) {
//     std::pair<abel::string_view, abel::string_view> tokens =
//         abel::StrSplit(text, ',');
//     if (!abel::parse_flag(tokens.first, &flag->my_flag_data.first, err))
//         return false;
//     if (!abel::parse_flag(tokens.second, &flag->my_flag_data.second, err))
//         return false;
//     return true;
//   }
//
//   // Similarly, for unparsing, we can simply invoke `abel::unparse_flag()` on
//   // the constituent types.
//   std::string abel_unparse_flag(const MyFlagType& flag) {
//     return abel::string_cat(abel::unparse_flag(flag.my_flag_data.first),
//                         ",",
//                         abel::unparse_flag(flag.my_flag_data.second));
//   }
#ifndef ABEL_FLAGS_MARSHALLING_H_
#define ABEL_FLAGS_MARSHALLING_H_

#include <string>
#include <vector>

#include <abel/asl/string_view.h>

namespace abel {

    namespace flags_internal {

// Overloads of `abel_parse_flag()` and `abel_unparse_flag()` for fundamental types.
        bool abel_parse_flag(abel::string_view, bool *, std::string *);

        bool abel_parse_flag(abel::string_view, short *, std::string *);           // NOLINT
        bool abel_parse_flag(abel::string_view, unsigned short *, std::string *);  // NOLINT
        bool abel_parse_flag(abel::string_view, int *, std::string *);             // NOLINT
        bool abel_parse_flag(abel::string_view, unsigned int *, std::string *);    // NOLINT
        bool abel_parse_flag(abel::string_view, long *, std::string *);            // NOLINT
        bool abel_parse_flag(abel::string_view, unsigned long *, std::string *);   // NOLINT
        bool abel_parse_flag(abel::string_view, long long *, std::string *);       // NOLINT
        bool abel_parse_flag(abel::string_view, unsigned long long *,             // NOLINT
                             std::string *);

        bool abel_parse_flag(abel::string_view, float *, std::string *);

        bool abel_parse_flag(abel::string_view, double *, std::string *);

        bool abel_parse_flag(abel::string_view, std::string *, std::string *);

        bool abel_parse_flag(abel::string_view, std::vector<std::string> *, std::string *);

        template<typename T>
        bool InvokeParseFlag(abel::string_view input, T *dst, std::string *err) {
            // Comment on next line provides a good compiler error message if T
            // does not have abel_parse_flag(abel::string_view, T*, std::string*).
            return abel_parse_flag(input, dst, err);  // Is T missing abel_parse_flag?
        }

// Strings and std:: containers do not have the same overload resolution
// considerations as fundamental types. Naming these 'abel_unparse_flag' means we
// can avoid the need for additional specializations of Unparse (below).
        std::string abel_unparse_flag(abel::string_view v);

        std::string abel_unparse_flag(const std::vector<std::string> &);

        template<typename T>
        std::string Unparse(const T &v) {
            // Comment on next line provides a good compiler error message if T does not
            // have unparse_flag.
            return abel_unparse_flag(v);  // Is T missing abel_unparse_flag?
        }

// Overloads for builtin types.
        std::string Unparse(bool v);

        std::string Unparse(short v);               // NOLINT
        std::string Unparse(unsigned short v);      // NOLINT
        std::string Unparse(int v);                 // NOLINT
        std::string Unparse(unsigned int v);        // NOLINT
        std::string Unparse(long v);                // NOLINT
        std::string Unparse(unsigned long v);       // NOLINT
        std::string Unparse(long long v);           // NOLINT
        std::string Unparse(unsigned long long v);  // NOLINT
        std::string Unparse(float v);

        std::string Unparse(double v);

    }  // namespace flags_internal

// parse_flag()
//
// Parses a string value into a flag value of type `T`. Do not add overloads of
// this function for your type directly; instead, add an `abel_parse_flag()`
// free function as documented above.
//
// Some implementations of `abel_parse_flag()` for types which consist of other,
// constituent types which already have abel flag support, may need to call
// `abel::parse_flag()` on those consituent string values. (See above.)
    template<typename T>
    ABEL_FORCE_INLINE bool parse_flag(abel::string_view input, T *dst, std::string *error) {
        return flags_internal::InvokeParseFlag(input, dst, error);
    }

// unparse_flag()
//
// Unparses a flag value of type `T` into a string value. Do not add overloads
// of this function for your type directly; instead, add an `abel_unparse_flag()`
// free function as documented above.
//
// Some implementations of `abel_unparse_flag()` for types which consist of other,
// constituent types which already have abel flag support, may want to call
// `abel::unparse_flag()` on those constituent types. (See above.)
    template<typename T>
    ABEL_FORCE_INLINE std::string unparse_flag(const T &v) {
        return flags_internal::Unparse(v);
    }

// Overloads for `abel::LogSeverity` can't (easily) appear alongside that type's
// definition because it is layered below flags.  See proper documentation in
// base/log_severity.h.
    enum class LogSeverity : int;

    bool abel_parse_flag(abel::string_view, abel::LogSeverity *, std::string *);

    std::string abel_unparse_flag(abel::LogSeverity);


}  // namespace abel

#endif  // ABEL_FLAGS_MARSHALLING_H_
