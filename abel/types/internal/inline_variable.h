//

#ifndef ABEL_TYPES_INTERNAL_INLINE_VARIABLE_EMULATION_H_
#define ABEL_TYPES_INTERNAL_INLINE_VARIABLE_EMULATION_H_

#include <type_traits>
#include <abel/meta/type_traits.h>

// File:
//   This file define a macro that allows the creation of or emulation of C++17
//   inline variables based on whether or not the feature is supported.

////////////////////////////////////////////////////////////////////////////////
// Macro: ABEL_INTERNAL_INLINE_CONSTEXPR(type, name, init)
//
// Description:
//   Expands to the equivalent of an inline constexpr instance of the specified
//   `type` and `name`, initialized to the value `init`. If the compiler being
//   used is detected as supporting actual inline variables as a language
//   feature, then the macro expands to an actual inline variable definition.
//
// Requires:
//   `type` is a type that is usable in an extern variable declaration.
//
// Requires: `name` is a valid identifier
//
// Requires:
//   `init` is an expression that can be used in the following definition:
//     constexpr type name = init;
//
// Usage:
//
//   // Equivalent to: `inline constexpr size_t variant_npos = -1;`
//   ABEL_INTERNAL_INLINE_CONSTEXPR(size_t, variant_npos, -1);
//
// Differences in implementation:
//   For a direct, language-level inline variable, decltype(name) will be the
//   type that was specified along with const qualification, whereas for
//   emulated inline variables, decltype(name) may be different (in practice
//   it will likely be a reference type).
////////////////////////////////////////////////////////////////////////////////

#ifdef __cpp_inline_variables

// Clang's -Wmissing-variable-declarations option erroneously warned that
// inline constexpr objects need to be pre-declared. This has now been fixed,
// but we will need to support this workaround for people building with older
// versions of clang.
//
// Bug: https://bugs.llvm.org/show_bug.cgi?id=35862
//
// Note:
//   identity_t is used here so that the const and name are in the
//   appropriate place for pointer types, reference types, function pointer
//   types, etc..
#if defined(__clang__)
#define ABEL_INTERNAL_EXTERN_DECL(type, name) \
  extern const ::abel::internal::identity_t<type> name;
#else  // Otherwise, just define the macro to do nothing.
#define ABEL_INTERNAL_EXTERN_DECL(type, name)
#endif  // defined(__clang__)

// See above comment at top of file for details.
#define ABEL_INTERNAL_INLINE_CONSTEXPR(type, name, init) \
  ABEL_INTERNAL_EXTERN_DECL(type, name)                  \
  ABEL_FORCE_INLINE constexpr ::abel::internal::identity_t<type> name = init

#else

// See above comment at top of file for details.
//
// Note:
//   identity_t is used here so that the const and name are in the
//   appropriate place for pointer types, reference types, function pointer
//   types, etc..
#define ABEL_INTERNAL_INLINE_CONSTEXPR(var_type, name, init)                  \
  template <class /*AbelInternalDummy*/ = void>                               \
  struct AbelInternalInlineVariableHolder##name {                             \
    static constexpr ::abel::internal::identity_t<var_type> kInstance = init; \
  };                                                                          \
                                                                              \
  template <class AbelInternalDummy>                                          \
  constexpr ::abel::internal::identity_t<var_type>                            \
      AbelInternalInlineVariableHolder##name<AbelInternalDummy>::kInstance;   \
                                                                              \
  static constexpr const ::abel::internal::identity_t<var_type>&              \
      name = /* NOLINT */                                                     \
      AbelInternalInlineVariableHolder##name<>::kInstance;                    \
  static_assert(sizeof(void (*)(decltype(name))) != 0,                        \
                "Silence unused variable warnings.")

#endif  // __cpp_inline_variables

#endif  // ABEL_TYPES_INTERNAL_INLINE_VARIABLE_EMULATION_H_
