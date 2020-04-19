

#include <abel/strings/ends_with.h>
#include <gtest/gtest.h>

namespace {

    TEST(MatchTest, ends_with_case) {
        EXPECT_TRUE(abel::ends_with_case("foo", "foo"));
        EXPECT_TRUE(abel::ends_with_case("foo", "Oo"));
        EXPECT_TRUE(abel::ends_with_case("foo", ""));
        EXPECT_FALSE(abel::ends_with_case("foo", "fooo"));
        EXPECT_FALSE(abel::ends_with_case("", "fo"));
    }

    TEST(MatchTest, ends_with) {
        const std::string s1("123\0abc", 7);
        const abel::string_view a("foobar");
        const abel::string_view b(s1);
        const abel::string_view e;
        EXPECT_TRUE(abel::ends_with(a, a));
        EXPECT_TRUE(abel::ends_with(a, "bar"));
        EXPECT_TRUE(abel::ends_with(a, e));
        EXPECT_TRUE(abel::ends_with(b, s1));
        EXPECT_TRUE(abel::ends_with(b, b));
        EXPECT_TRUE(abel::ends_with(b, e));
        EXPECT_TRUE(abel::ends_with(e, ""));
        EXPECT_FALSE(abel::ends_with(a, b));
        EXPECT_FALSE(abel::ends_with(b, a));
        EXPECT_FALSE(abel::ends_with(e, a));
    }

}
