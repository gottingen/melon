
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_BASE_PROFILE_ATTRIBUTE_H_
#define MELON_BASE_PROFILE_ATTRIBUTE_H_

#include "melon/base/profile/compiler.h"

// Annotate a function indicating it should not be inlined.
// Use like:
//   NOINLINE void DoStuff() { ... }
#ifndef MELON_NO_INLINE
#if defined(MELON_COMPILER_GNUC)
#define MELON_NO_INLINE __attribute__((noinline))
#elif defined(MELON_COMPILER_MSVC)
#define MELON_NO_INLINE __declspec(noinline)
#else
#define MELON_NO_INLINE
#endif
#endif  // MELON_NO_INLINE

#ifndef MELON_FORCE_INLINE
#if defined(MELON_COMPILER_MSVC)
#define MELON_FORCE_INLINE    __forceinline
#else
#define MELON_FORCE_INLINE inline __attribute__((always_inline))
#endif
#endif  // MELON_FORCE_INLINE

#ifndef MELON_ALLOW_UNUSED
#if defined(MELON_COMPILER_GNUC)
#define MELON_ALLOW_UNUSED __attribute__((unused))
#else
#define MELON_ALLOW_UNUSED
#endif
#endif  // MELON_ALLOW_UNUSED

#ifndef MELON_HAVE_ATTRIBUTE
#ifdef __has_attribute
#define MELON_HAVE_ATTRIBUTE(x) __has_attribute(x)
#else
#define MELON_HAVE_ATTRIBUTE(x) 0
#endif
#endif  // MELON_HAVE_ATTRIBUTE

#ifndef MELON_MUST_USE_RESULT
#if MELON_HAVE_ATTRIBUTE(nodiscard)
#define MELON_MUST_USE_RESULT [[nodiscard]]
#elif defined(__clang__) && MELON_HAVE_ATTRIBUTE(warn_unused_result)
#define MELON_MUST_USE_RESULT __attribute__((warn_unused_result))
#else
#define MELON_MUST_USE_RESULT
#endif
#endif  // MELON_MUST_USE_RESULT

#define MELON_ARRAY_SIZE(array) \
  (sizeof(::melon::base::base_internal::ArraySizeHelper(array)))

namespace melon::base::base_internal {
    template<typename T, size_t N>
    auto ArraySizeHelper(const T (&array)[N]) -> char (&)[N];
}  // namespace melon::base::base_internal

// MELON_DEPRECATED void dont_call_me_anymore(int arg);
// ...
// warning: 'void dont_call_me_anymore(int)' is deprecated
#if defined(MELON_COMPILER_GNUC) || defined(MELON_COMPILER_CLANG)
# define MELON_DEPRECATED __attribute__((deprecated))
#elif defined(MELON_COMPILER_MSVC)
# define MELON_DEPRECATED __declspec(deprecated)
#else
# define MELON_DEPRECATED
#endif

// Mark function as weak. This is GCC only feature.
#if defined(MELON_COMPILER_GNUC) || defined(MELON_COMPILER_CLANG)
# define MELON_WEAK __attribute__((weak))
#else
# define MELON_WEAK
#endif

#if (defined(MELON_COMPILER_GNUC) || defined(MELON_COMPILER_CLANG)) && __cplusplus >= 201103
#define MELON_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define MELON_WARN_UNUSED_RESULT
#endif

#ifdef _MSC_VER
# define MELON_CACHELINE_ALIGNMENT __declspec(align(MELON_CACHE_LINE_SIZE))
#endif /* _MSC_VER */

#ifdef __GNUC__
# define MELON_CACHELINE_ALIGNMENT __attribute__((aligned(MELON_CACHE_LINE_SIZE)))
#endif /* __GNUC__ */

#ifndef MELON_CACHELINE_ALIGNMENT
# define MELON_CACHELINE_ALIGNMENT /*MELON_CACHELINE_ALIGNMENT*/
#endif


#if defined(MELON_COMPILER_GNUC) || defined(MELON_COMPILER_CLANG)
#  if defined(__cplusplus)
#    define MELON_LIKELY(expr) (__builtin_expect(!!(expr), true))
#    define MELON_UNLIKELY(expr) (__builtin_expect(!!(expr), false))
#  else
#    define MELON_LIKELY(expr) (__builtin_expect(!!(expr), 1))
#    define MELON_UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#  endif
#else
#  define MELON_LIKELY(expr) (expr)
#  define MELON_UNLIKELY(expr) (expr)
#endif


// Specify memory alignment for structs, classes, etc.
// Use like:
//   class MELON_ALIGN_AS(16) MyClass { ... }
//   MELON_ALIGN_AS(16) int array[4];
#if defined(MELON_COMPILER_MSVC)
#define MELON_ALIGN_AS(byte_alignment) __declspec(align(byte_alignment))
#elif defined(MELON_COMPILER_GNUC) || defined(MELON_COMPILER_CLANG)
#define MELON_ALIGN_AS(byte_alignment) __attribute__((aligned(byte_alignment)))
#endif

// Return the byte alignment of the given type (available at compile time).  Use
// sizeof(type) prior to checking __alignof to workaround Visual C++ bug:
// http://goo.gl/isH0C
// Use like:
//   MELON_ALIGN_OF(int32_t)  // this would be 4
#if defined(MELON_COMPILER_MSVC)
#define MELON_ALIGN_OF(type) (sizeof(type) - sizeof(type) + __alignof(type))
#elif defined(MELON_COMPILER_GNUC) || defined(MELON_COMPILER_CLANG)
#define MELON_ALIGN_OF(type) __alignof__(type)
#endif


// MELON_NO_SANITIZE_MEMORY
//
// Tells the  MemorySanitizer to relax the handling of a given function. All
// "Use of uninitialized value" warnings from such functions will be suppressed,
// and all values loaded from memory will be considered fully initialized.
// This attribute is similar to the ADDRESS_SANITIZER attribute above, but deals
// with initialized-ness rather than addressability issues.
// NOTE: MemorySanitizer(msan) is supported by Clang but not GCC.
#if defined(__clang__)
#define MELON_NO_SANITIZE_MEMORY __attribute__((no_sanitize_memory))
#else
#define MELON_NO_SANITIZE_MEMORY
#endif


// MELON_NO_SANITIZE_THREAD
//
// Tells the ThreadSanitizer to not instrument a given function.
// NOTE: GCC supports ThreadSanitizer(tsan) since 4.8.
// https://gcc.gnu.org/gcc-4.8/changes.html
#if defined(__GNUC__)
#define MELON_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#else
#define MELON_NO_SANITIZE_THREAD
#endif


// MELON_NO_SANITIZE_ADDRESS
//
// Tells the AddressSanitizer (or other memory testing tools) to ignore a given
// function. Useful for cases when a function reads random locations on stack,
// calls _exit from a cloned subprocess, deliberately accesses buffer
// out of bounds or does other scary things with memory.
// NOTE: GCC supports AddressSanitizer(asan) since 4.8.
// https://gcc.gnu.org/gcc-4.8/changes.html
#if defined(__GNUC__)
#define MELON_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
#define MELON_NO_SANITIZE_ADDRESS
#endif

#if MELON_COMPILER_HAS_FEATURE(thread_sanitizer) || __SANITIZE_THREAD__
#define MELON_SANITIZE_THREAD 1
#endif

#if defined(MELON_SANITIZE_THREAD)
#define MELON_IFDEF_THREAD_SANITIZER(X) X
#else
#define MELON_IFDEF_THREAD_SANITIZER(X)
#endif


// MELON_HOT, MELON_COLD
//
// Tells GCC that a function is hot or cold. GCC can use this information to
// improve static analysis, i.e. a conditional branch to a cold function
// is likely to be not-taken.
// This annotation is used for function declarations.
//
// Example:
//
//   int foo() MELON_HOT;
#if MELON_COMPILER_HAS_ATTRIBUTE(hot) || (defined(__GNUC__) && !defined(__clang__))
#define MELON_HOT __attribute__((hot))
#else
#define MELON_HOT
#endif

#if MELON_COMPILER_HAS_ATTRIBUTE(cold) || (defined(__GNUC__) && !defined(__clang__))
#define MELON_COLD __attribute__((cold))
#else
#define MELON_COLD
#endif



// ------------------------------------------------------------------------
// MELON_UNUSED
//
// Makes compiler warnings about unused variables go away.
//
// Example usage:
//    void Function(int x)
//    {
//        int y;
//        MELON_UNUSED(x);
//        MELON_UNUSED(y);
//    }
//
#ifndef MELON_UNUSED
// The EDG solution below is pretty weak and needs to be augmented or replaced.
// It can't handle the C language, is limited to places where template declarations
// can be used, and requires the type x to be usable as a functions reference argument.
#if defined(__cplusplus) && defined(__EDG__)
namespace melon:: base_internal {
template <typename T>
inline void melon_macro_unused(T const volatile & x) { (void)x; }
}
#define MELON_UNUSED(x) melon:: base_internal::melon_macro_unused(x)
#else
#define MELON_UNUSED(x) (void)x
#endif
#endif

// ------------------------------------------------------------------------
// MELON_HIDDEN
//

#ifndef MELON_HIDDEN
    #if  defined(MELON_COMPILER_GNUC) || defined(MELON_COMPILER_CLANG)
        #define MELON_HIDDEN __attribute__((visibility("hidden")))
    #else
        #define MELON_HIDDEN
    #endif
#endif  // MELON_HIDDEN

#endif // MELON_BASE_PROFILE_ATTRIBUTE_H_
