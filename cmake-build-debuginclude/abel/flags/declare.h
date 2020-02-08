//
//
// -----------------------------------------------------------------------------
// File: declare.h
// -----------------------------------------------------------------------------
//
// This file defines the ABEL_DECLARE_FLAG macro, allowing you to declare an
// `abel::Flag` for use within a translation unit. You should place this
// declaration within the header file associated with the .cc file that defines
// and owns the `Flag`.

#ifndef ABEL_FLAGS_DECLARE_H_
#define ABEL_FLAGS_DECLARE_H_

#include <abel/strings/string_view.h>

namespace abel {

namespace flags_internal {

// abel::Flag<T> represents a flag of type 'T' created by ABEL_FLAG.
template <typename T>
class Flag;

}  // namespace flags_internal

// Flag
//
// Forward declaration of the `abel::Flag` type for use in defining the macro.
#if defined(_MSC_VER) && !defined(__clang__)
template <typename T>
class Flag;
#else
template <typename T>
using Flag = flags_internal::Flag<T>;
#endif


}  // namespace abel

// ABEL_DECLARE_FLAG()
//
// This macro is a convenience for declaring use of an `abel::Flag` within a
// translation unit. This macro should be used within a header file to
// declare usage of the flag within any .cc file including that header file.
//
// The ABEL_DECLARE_FLAG(type, name) macro expands to:
//
//   extern abel::Flag<type> FLAGS_name;
#define ABEL_DECLARE_FLAG(type, name) extern ::abel::Flag<type> FLAGS_##name

#endif  // ABEL_FLAGS_DECLARE_H_
