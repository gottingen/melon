//
// Created by liyinbin on 2020/1/26.
//

#ifndef TEST_TESTING_GTEST_ASSERT_H_
#define TEST_TESTING_GTEST_ASSERT_H_

#include <stdexcept>

class AssertionFailure : public std::logic_error {
public:
    explicit AssertionFailure(const char *message) : std::logic_error(message) {}
};

#define FMT_ASSERT(condition, message) \
  if (!(condition)) throw AssertionFailure(message);

#include <test/testing/gtest_extra.h>

// Expects an assertion failure.
#define EXPECT_ASSERT(stmt, message) \
  EXPECT_THROW_MSG(stmt, AssertionFailure, message)

#endif //TEST_TESTING_GTEST_ASSERT_H_
