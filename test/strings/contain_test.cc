
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "melon/strings/contain.h"
#include "melon/strings/starts_with.h"
#include "melon/strings/ends_with.h"
#include "testing/gtest_wrap.h"

namespace {


    TEST(MatchTest, Contains) {
        std::string_view a("abcdefg");
        std::string_view b("abcd");
        std::string_view c("efg");
        std::string_view d("gh");
        EXPECT_TRUE(melon::string_contains(a, a));
        EXPECT_TRUE(melon::string_contains(a, b));
        EXPECT_TRUE(melon::string_contains(a, c));
        EXPECT_FALSE(melon::string_contains(a, d));
        EXPECT_TRUE(melon::string_contains("", ""));
        EXPECT_TRUE(melon::string_contains("abc", ""));
        EXPECT_FALSE(melon::string_contains("", "a"));
    }

    TEST(MatchTest, ContainsNull) {
        const std::string s = "foo";
        const char *cs = "foo";
        const std::string_view sv("foo");
        const std::string_view sv2("foo\0bar", 4);
        EXPECT_EQ(s, "foo");
        EXPECT_EQ(sv, "foo");
        EXPECT_NE(sv2, "foo");
        EXPECT_TRUE(melon::ends_with(s, sv));
        EXPECT_TRUE(melon::starts_with(cs, sv));
        EXPECT_TRUE(melon::string_contains(cs, sv));
        EXPECT_FALSE(melon::string_contains(cs, sv2));
    }
}
