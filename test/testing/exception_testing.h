//

// Testing utilities for ABEL types which throw exceptions.

#ifndef ABEL_BASE_INTERNAL_EXCEPTION_TESTING_H_
#define ABEL_BASE_INTERNAL_EXCEPTION_TESTING_H_

#include <gtest/gtest.h>
#include <abel/base/profile.h>

// ABEL_BASE_INTERNAL_EXPECT_FAIL tests either for a specified thrown exception
// if exceptions are enabled, or for death with a specified text in the error
// message
#ifdef ABEL_HAVE_EXCEPTIONS

#define ABEL_BASE_INTERNAL_EXPECT_FAIL(expr, exception_t, text) \
  EXPECT_THROW(expr, exception_t)

#elif defined(__ANDROID__)
// Android asserts do not log anywhere that gtest can currently inspect.
// So we expect exit, but cannot match the message.
#define ABEL_BASE_INTERNAL_EXPECT_FAIL(expr, exception_t, text) \
  EXPECT_DEATH(expr, ".*")
#else
#define ABEL_BASE_INTERNAL_EXPECT_FAIL(expr, exception_t, text) \
  EXPECT_DEATH_IF_SUPPORTED(expr, text)

#endif

#endif  // ABEL_BASE_INTERNAL_EXCEPTION_TESTING_H_
