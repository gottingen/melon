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

#ifndef ABEL_CONFIG_DECLARE_H_
#define ABEL_CONFIG_DECLARE_H_

#include <abel/asl/string_view.h>

namespace abel {

    namespace flags_internal {

// abel::Flag<T> represents a flag of type 'T' created by ABEL_FLAG.
        template<typename T>
        class abel_flag;

    }  // namespace flags_internal

// Flag
//
// Forward declaration of the `abel::Flag` type for use in defining the macro.
#if defined(_MSC_VER) && !defined(__clang__)
    template <typename T>
    class abel_flag;
#else
    template<typename T>
    using abel_flag = flags_internal::abel_flag<T>;
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
#define ABEL_DECLARE_FLAG(type, name) extern ::abel::abel_flag<type> FLAGS_##name

#endif  // ABEL_CONFIG_DECLARE_H_
