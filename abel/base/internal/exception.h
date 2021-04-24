// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_BASE_INTERNAL_EXCEPTION_H_
#define ABEL_BASE_INTERNAL_EXCEPTION_H_

#include "abel/base/internal/have.h"
#include <cassert>

#ifndef ABEL_ASSERT_MSG
#define ABEL_ASSERT_MSG(e, m) assert((e)&&(m))
#endif

// ABEL_ASSERT()
//
// In C++11, `assert` can't be used portably within constexpr functions.
// ABEL_ASSERT functions as a runtime assert but works in C++11 constexpr
// functions.  Example:
//
// constexpr double Divide(double a, double b) {
//   return ABEL_ASSERT(b != 0), a / b;
// }
//
// This macro is inspired by
// https://akrzemi1.wordpress.com/2017/05/18/asserts-in-constexpr-functions/
#if defined(NDEBUG)
#define ABEL_ASSERT(expr) \
  (false ? static_cast<void>(expr) : static_cast<void>(0))
#else
#define ABEL_ASSERT(expr)                           \
  (ABEL_LIKELY((expr)) ? static_cast<void>(0) \
                             : [] { assert(false && #expr); }())  // NOLINT
#endif

#ifndef ABEL_FAIL_MSG
#define ABEL_FAIL_MSG(m) ABEL_ASSERT_MSG(false, (m))

#endif

#ifndef ABEL_STATIC_ASSERT_MSG
#define ABEL_STATIC_ASSERT_MSG(e, m) static_assert(e, #m)
#endif

#ifndef ABEL_STATIC_ASSERT
    #define ABEL_STATIC_ASSERT(e) static_assert(e, #e)
#endif


#ifdef ABEL_HAVE_EXCEPTIONS
    #define ABEL_TRY try
    #define ABEL_CATCH(...) catch(__VA_ARGS__)
    #define ABEL_CATCH_ALL() catch(...)
    #define ABEL_RETHROW throw
    #define ABEL_THROW(expr) throw (expr)
#else
    #define ABEL_TRY ABEL_CONSTEXPR_IF (true)
    #define ABEL_CATCH(...) else ABEL_CONSTEXPR_IF (false)
    #define ABEL_CATCH_ALL() else ABEL_CONSTEXPR_IF (false)
    #define ABEL_RETHROW do {} while (false)
    #define ABEL_THROW(expr) ABEL_ASSERT(expr)
#endif

#endif  // ABEL_BASE_INTERNAL_EXCEPTION_H_
