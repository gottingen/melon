//

#include <abel/strings/match.h>

#include <gtest/gtest.h>

namespace {

TEST(MatchTest, starts_with) {
  const std::string s1("123\0abc", 7);
  const abel::string_view a("foobar");
  const abel::string_view b(s1);
  const abel::string_view e;
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
  const char* cs = "foo";
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

TEST(MatchTest, EqualsIgnoreCase) {
  std::string text = "the";
  abel::string_view data(text);

  EXPECT_TRUE(abel::EqualsIgnoreCase(data, "The"));
  EXPECT_TRUE(abel::EqualsIgnoreCase(data, "THE"));
  EXPECT_TRUE(abel::EqualsIgnoreCase(data, "the"));
  EXPECT_FALSE(abel::EqualsIgnoreCase(data, "Quick"));
  EXPECT_FALSE(abel::EqualsIgnoreCase(data, "then"));
}

TEST(MatchTest, starts_with_case) {
  EXPECT_TRUE(abel::starts_with_case("foo", "foo"));
  EXPECT_TRUE(abel::starts_with_case("foo", "Fo"));
  EXPECT_TRUE(abel::starts_with_case("foo", ""));
  EXPECT_FALSE(abel::starts_with_case("foo", "fooo"));
  EXPECT_FALSE(abel::starts_with_case("", "fo"));
}

TEST(MatchTest, ends_with_case) {
  EXPECT_TRUE(abel::ends_with_case("foo", "foo"));
  EXPECT_TRUE(abel::ends_with_case("foo", "Oo"));
  EXPECT_TRUE(abel::ends_with_case("foo", ""));
  EXPECT_FALSE(abel::ends_with_case("foo", "fooo"));
  EXPECT_FALSE(abel::ends_with_case("", "fo"));
}

}  // namespace
