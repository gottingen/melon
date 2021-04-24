// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/strings/starts_with.h"
#include "gtest/gtest.h"

namespace {


    TEST(MatchTest, starts_with) {
        const std::string s1("123\0abc", 7);
        const std::string_view a("foobar");
        const std::string_view b(s1);
        const std::string_view e;
        EXPECT_TRUE(abel::starts_with(a, a));
        EXPECT_TRUE(abel::starts_with(a, "foo"));
        EXPECT_TRUE(abel::starts_with(a, e));
        EXPECT_TRUE(abel::starts_with(b, s1));
        EXPECT_TRUE(abel::starts_with(b, b));
        EXPECT_TRUE(abel::starts_with(b, e));
        EXPECT_TRUE(abel::starts_with(e, ""));
        EXPECT_FALSE(abel::starts_with(a, b));
        EXPECT_FALSE(abel::starts_with(b, a));
        EXPECT_FALSE(abel::starts_with(e, a));
    }


    TEST(MatchTest, starts_with_case) {
        EXPECT_TRUE(abel::starts_with_case("foo", "foo"));
        EXPECT_TRUE(abel::starts_with_case("foo", "Fo"));
        EXPECT_TRUE(abel::starts_with_case("foo", ""));
        EXPECT_FALSE(abel::starts_with_case("foo", "fooo"));
        EXPECT_FALSE(abel::starts_with_case("", "fo"));
    }

}
