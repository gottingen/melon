//
// Created by liyinbin on 2020/1/26.
//

#include <abel/format/core.h>
#include <gtest/gtest.h>

#if GTEST_HAS_DEATH_TEST
# define EXPECT_DEBUG_DEATH_IF_SUPPORTED(statement, regex) \
    EXPECT_DEBUG_DEATH(statement, regex)
#else
# define EXPECT_DEBUG_DEATH_IF_SUPPORTED(statement, regex) \
    GTEST_UNSUPPORTED_DEATH_TEST_(statement, regex, )
#endif

TEST(AssertTest, Fail) {
    EXPECT_DEBUG_DEATH_IF_SUPPORTED(
        FMT_ASSERT(false, "don't panic!"), "don't panic!");
}