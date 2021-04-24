// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/strings/contain.h"
#include "abel/strings/starts_with.h"
#include "abel/strings/ends_with.h"
#include "gtest/gtest.h"

namespace {


    TEST(MatchTest, Contains) {
        std::string_view a("abcdefg");
        std::string_view b("abcd");
        std::string_view c("efg");
        std::string_view d("gh");
        EXPECT_TRUE(abel::string_contains(a, a));
        EXPECT_TRUE(abel::string_contains(a, b));
        EXPECT_TRUE(abel::string_contains(a, c));
        EXPECT_FALSE(abel::string_contains(a, d));
        EXPECT_TRUE(abel::string_contains("", ""));
        EXPECT_TRUE(abel::string_contains("abc", ""));
        EXPECT_FALSE(abel::string_contains("", "a"));
    }

    TEST(MatchTest, ContainsNull) {
        const std::string s = "foo";
        const char *cs = "foo";
        const std::string_view sv("foo");
        const std::string_view sv2("foo\0bar", 4);
        EXPECT_EQ(s, "foo");
        EXPECT_EQ(sv, "foo");
        EXPECT_NE(sv2, "foo");
        EXPECT_TRUE(abel::ends_with(s, sv));
        EXPECT_TRUE(abel::starts_with(cs, sv));
        EXPECT_TRUE(abel::string_contains(cs, sv));
        EXPECT_FALSE(abel::string_contains(cs, sv2));
    }
}
