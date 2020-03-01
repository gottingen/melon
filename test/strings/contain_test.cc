
#include <abel/strings/contain.h>
#include <abel/strings/starts_with.h>
#include <abel/strings/ends_with.h>
#include <gtest/gtest.h>

namespace {


    TEST(MatchTest, Contains) {
        abel::string_view a("abcdefg");
        abel::string_view b("abcd");
        abel::string_view c("efg");
        abel::string_view d("gh");
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
        const abel::string_view sv("foo");
        const abel::string_view sv2("foo\0bar", 4);
        EXPECT_EQ(s, "foo");
        EXPECT_EQ(sv, "foo");
        EXPECT_NE(sv2, "foo");
        EXPECT_TRUE(abel::ends_with(s, sv));
        EXPECT_TRUE(abel::starts_with(cs, sv));
        EXPECT_TRUE(abel::string_contains(cs, sv));
        EXPECT_FALSE(abel::string_contains(cs, sv2));
    }
}
