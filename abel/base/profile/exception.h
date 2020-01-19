//
// Created by liyinbin on 2019/12/26.
//

#ifndef ABEL_BASE_PROFILE_EXCEPTION_H_
#define ABEL_BASE_PROFILE_EXCEPTION_H_
#include <abel/base/profile/have.h>
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
    #define ABEL_STATIC_ASSERT_MSG(e,m) static_assert(e, #m)
#endif

#ifndef ABEL_STATIC_ASSERT
    #define ABEL_STATIC_ASSERT(e) static_assert(e, #e)
#endif
#ifndef ABEL_THROW
    #define ABEL_THROW(x) do { static_cast<void>(sizeof(x)); assert(false); } while(false);
#endif

#ifdef ABEL_HAVE_EXCEPTIONS
#define ABEL_INTERNAL_TRY try
#define ABEL_INTERNAL_CATCH_ANY catch (...)
#define ABEL_INTERNAL_RETHROW do { throw; } while (false)
#else  // ABEL_HAVE_EXCEPTIONS
#define ABEL_INTERNAL_TRY if (true)
#define ABEL_INTERNAL_CATCH_ANY else if (false)
#define ABEL_INTERNAL_RETHROW do {} while (false)
#endif  // ABEL_HAVE_EXCEPTIONS

#endif //ABEL_BASE_PROFILE_EXCEPTION_H_
