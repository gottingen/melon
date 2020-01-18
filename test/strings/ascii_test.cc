//

#include <abel/strings/ascii.h>

#include <cctype>
#include <clocale>
#include <cstring>
#include <string>

#include <gtest/gtest.h>
#include <abel/base/profile.h>
#include <abel/base/profile.h>

namespace {

TEST(AsciiIsFoo, All) {
  for (int i = 0; i < 256; i++) {
    if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z'))
      EXPECT_TRUE(abel::ascii_isalpha(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!abel::ascii_isalpha(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if ((i >= '0' && i <= '9'))
      EXPECT_TRUE(abel::ascii_isdigit(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!abel::ascii_isdigit(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (abel::ascii_isalpha(i) || abel::ascii_isdigit(i))
      EXPECT_TRUE(abel::ascii_isalnum(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!abel::ascii_isalnum(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i != '\0' && strchr(" \r\n\t\v\f", i))
      EXPECT_TRUE(abel::ascii_isspace(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!abel::ascii_isspace(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i >= 32 && i < 127)
      EXPECT_TRUE(abel::ascii_isprint(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!abel::ascii_isprint(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (abel::ascii_isprint(i) && !abel::ascii_isspace(i) &&
        !abel::ascii_isalnum(i))
      EXPECT_TRUE(abel::ascii_ispunct(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!abel::ascii_ispunct(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i == ' ' || i == '\t')
      EXPECT_TRUE(abel::ascii_isblank(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!abel::ascii_isblank(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i < 32 || i == 127)
      EXPECT_TRUE(abel::ascii_iscntrl(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!abel::ascii_iscntrl(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (abel::ascii_isdigit(i) || (i >= 'A' && i <= 'F') ||
        (i >= 'a' && i <= 'f'))
      EXPECT_TRUE(abel::ascii_isxdigit(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!abel::ascii_isxdigit(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i > 32 && i < 127)
      EXPECT_TRUE(abel::ascii_isgraph(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!abel::ascii_isgraph(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i >= 'A' && i <= 'Z')
      EXPECT_TRUE(abel::ascii_isupper(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!abel::ascii_isupper(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i >= 'a' && i <= 'z')
      EXPECT_TRUE(abel::ascii_islower(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!abel::ascii_islower(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 128; i++) {
    EXPECT_TRUE(abel::ascii_isascii(i)) << ": failed on " << i;
  }
  for (int i = 128; i < 256; i++) {
    EXPECT_TRUE(!abel::ascii_isascii(i)) << ": failed on " << i;
  }

  // The official is* functions don't accept negative signed chars, but
  // our abel::ascii_is* functions do.
  for (int i = 0; i < 256; i++) {
    signed char sc = static_cast<signed char>(static_cast<unsigned char>(i));
    EXPECT_EQ(abel::ascii_isalpha(i), abel::ascii_isalpha(sc)) << i;
    EXPECT_EQ(abel::ascii_isdigit(i), abel::ascii_isdigit(sc)) << i;
    EXPECT_EQ(abel::ascii_isalnum(i), abel::ascii_isalnum(sc)) << i;
    EXPECT_EQ(abel::ascii_isspace(i), abel::ascii_isspace(sc)) << i;
    EXPECT_EQ(abel::ascii_ispunct(i), abel::ascii_ispunct(sc)) << i;
    EXPECT_EQ(abel::ascii_isblank(i), abel::ascii_isblank(sc)) << i;
    EXPECT_EQ(abel::ascii_iscntrl(i), abel::ascii_iscntrl(sc)) << i;
    EXPECT_EQ(abel::ascii_isxdigit(i), abel::ascii_isxdigit(sc)) << i;
    EXPECT_EQ(abel::ascii_isprint(i), abel::ascii_isprint(sc)) << i;
    EXPECT_EQ(abel::ascii_isgraph(i), abel::ascii_isgraph(sc)) << i;
    EXPECT_EQ(abel::ascii_isupper(i), abel::ascii_isupper(sc)) << i;
    EXPECT_EQ(abel::ascii_islower(i), abel::ascii_islower(sc)) << i;
    EXPECT_EQ(abel::ascii_isascii(i), abel::ascii_isascii(sc)) << i;
  }
}

// Checks that abel::ascii_isfoo returns the same value as isfoo in the C
// locale.
TEST(AsciiIsFoo, SameAsIsFoo) {
#ifndef __ANDROID__
  // temporarily change locale to C. It should already be C, but just for safety
  const char* old_locale = setlocale(LC_CTYPE, "C");
  ASSERT_TRUE(old_locale != nullptr);
#endif

  for (int i = 0; i < 256; i++) {
    EXPECT_EQ(isalpha(i) != 0, abel::ascii_isalpha(i)) << i;
    EXPECT_EQ(isdigit(i) != 0, abel::ascii_isdigit(i)) << i;
    EXPECT_EQ(isalnum(i) != 0, abel::ascii_isalnum(i)) << i;
    EXPECT_EQ(isspace(i) != 0, abel::ascii_isspace(i)) << i;
    EXPECT_EQ(ispunct(i) != 0, abel::ascii_ispunct(i)) << i;
    EXPECT_EQ(isblank(i) != 0, abel::ascii_isblank(i)) << i;
    EXPECT_EQ(iscntrl(i) != 0, abel::ascii_iscntrl(i)) << i;
    EXPECT_EQ(isxdigit(i) != 0, abel::ascii_isxdigit(i)) << i;
    EXPECT_EQ(isprint(i) != 0, abel::ascii_isprint(i)) << i;
    EXPECT_EQ(isgraph(i) != 0, abel::ascii_isgraph(i)) << i;
    EXPECT_EQ(isupper(i) != 0, abel::ascii_isupper(i)) << i;
    EXPECT_EQ(islower(i) != 0, abel::ascii_islower(i)) << i;
    EXPECT_EQ(isascii(i) != 0, abel::ascii_isascii(i)) << i;
  }

#ifndef __ANDROID__
  // restore the old locale.
  ASSERT_TRUE(setlocale(LC_CTYPE, old_locale));
#endif
}

TEST(AsciiToFoo, All) {
#ifndef __ANDROID__
  // temporarily change locale to C. It should already be C, but just for safety
  const char* old_locale = setlocale(LC_CTYPE, "C");
  ASSERT_TRUE(old_locale != nullptr);
#endif

  for (int i = 0; i < 256; i++) {
    if (abel::ascii_islower(i))
      EXPECT_EQ(abel::ascii_toupper(i), 'A' + (i - 'a')) << i;
    else
      EXPECT_EQ(abel::ascii_toupper(i), static_cast<char>(i)) << i;

    if (abel::ascii_isupper(i))
      EXPECT_EQ(abel::ascii_tolower(i), 'a' + (i - 'A')) << i;
    else
      EXPECT_EQ(abel::ascii_tolower(i), static_cast<char>(i)) << i;

    // These CHECKs only hold in a C locale.
    EXPECT_EQ(static_cast<char>(tolower(i)), abel::ascii_tolower(i)) << i;
    EXPECT_EQ(static_cast<char>(toupper(i)), abel::ascii_toupper(i)) << i;

    // The official to* functions don't accept negative signed chars, but
    // our abel::ascii_to* functions do.
    signed char sc = static_cast<signed char>(static_cast<unsigned char>(i));
    EXPECT_EQ(abel::ascii_tolower(i), abel::ascii_tolower(sc)) << i;
    EXPECT_EQ(abel::ascii_toupper(i), abel::ascii_toupper(sc)) << i;
  }
#ifndef __ANDROID__
  // restore the old locale.
  ASSERT_TRUE(setlocale(LC_CTYPE, old_locale));
#endif
}

TEST(AsciiStrTo, Lower) {
  const char buf[] = "ABCDEF";
  const std::string str("GHIJKL");
  const std::string str2("MNOPQR");
  const abel::string_view sp(str2);

  EXPECT_EQ("abcdef", abel::string_to_lower(buf));
  EXPECT_EQ("ghijkl", abel::string_to_lower(str));
  EXPECT_EQ("mnopqr", abel::string_to_lower(sp));

  char mutable_buf[] = "Mutable";
  std::transform(mutable_buf, mutable_buf + strlen(mutable_buf),
                 mutable_buf, abel::ascii_tolower);
  EXPECT_STREQ("mutable", mutable_buf);
}

TEST(AsciiStrTo, Upper) {
  const char buf[] = "abcdef";
  const std::string str("ghijkl");
  const std::string str2("mnopqr");
  const abel::string_view sp(str2);

  EXPECT_EQ("ABCDEF", abel::string_to_upper(buf));
  EXPECT_EQ("GHIJKL", abel::string_to_upper(str));
  EXPECT_EQ("MNOPQR", abel::string_to_upper(sp));

  char mutable_buf[] = "Mutable";
  std::transform(mutable_buf, mutable_buf + strlen(mutable_buf),
                 mutable_buf, abel::ascii_toupper);
  EXPECT_STREQ("MUTABLE", mutable_buf);
}

TEST(trim_left, FromStringView) {
  EXPECT_EQ(abel::string_view{},
            abel::trim_left(abel::string_view{}));
  EXPECT_EQ("foo", abel::trim_left({"foo"}));
  EXPECT_EQ("foo", abel::trim_left({"\t  \n\f\r\n\vfoo"}));
  EXPECT_EQ("foo foo\n ",
            abel::trim_left({"\t  \n\f\r\n\vfoo foo\n "}));
  EXPECT_EQ(abel::string_view{}, abel::trim_left(
                                     {"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(trim_left, InPlace) {
  std::string str;

  abel::trim_left(&str);
  EXPECT_EQ("", str);

  str = "foo";
  abel::trim_left(&str);
  EXPECT_EQ("foo", str);

  str = "\t  \n\f\r\n\vfoo";
  abel::trim_left(&str);
  EXPECT_EQ("foo", str);

  str = "\t  \n\f\r\n\vfoo foo\n ";
  abel::trim_left(&str);
  EXPECT_EQ("foo foo\n ", str);

  str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
  abel::trim_left(&str);
  EXPECT_EQ(abel::string_view{}, str);
}

TEST(trim_right, FromStringView) {
  EXPECT_EQ(abel::string_view{},
            abel::trim_right(abel::string_view{}));
  EXPECT_EQ("foo", abel::trim_right({"foo"}));
  EXPECT_EQ("foo", abel::trim_right({"foo\t  \n\f\r\n\v"}));
  EXPECT_EQ(" \nfoo foo",
            abel::trim_right({" \nfoo foo\t  \n\f\r\n\v"}));
  EXPECT_EQ(abel::string_view{}, abel::trim_right(
                                     {"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(trim_right, InPlace) {
  std::string str;

  abel::trim_right(&str);
  EXPECT_EQ("", str);

  str = "foo";
  abel::trim_right(&str);
  EXPECT_EQ("foo", str);

  str = "foo\t  \n\f\r\n\v";
  abel::trim_right(&str);
  EXPECT_EQ("foo", str);

  str = " \nfoo foo\t  \n\f\r\n\v";
  abel::trim_right(&str);
  EXPECT_EQ(" \nfoo foo", str);

  str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
  abel::trim_right(&str);
  EXPECT_EQ(abel::string_view{}, str);
}

TEST(trim_all, FromStringView) {
  EXPECT_EQ(abel::string_view{},
            abel::trim_all(abel::string_view{}));
  EXPECT_EQ("foo", abel::trim_all({"foo"}));
  EXPECT_EQ("foo",
            abel::trim_all({"\t  \n\f\r\n\vfoo\t  \n\f\r\n\v"}));
  EXPECT_EQ("foo foo", abel::trim_all(
                           {"\t  \n\f\r\n\vfoo foo\t  \n\f\r\n\v"}));
  EXPECT_EQ(abel::string_view{},
            abel::trim_all({"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(trim_all, InPlace) {
  std::string str;

  abel::trim_all(&str);
  EXPECT_EQ("", str);

  str = "foo";
  abel::trim_all(&str);
  EXPECT_EQ("foo", str);

  str = "\t  \n\f\r\n\vfoo\t  \n\f\r\n\v";
  abel::trim_all(&str);
  EXPECT_EQ("foo", str);

  str = "\t  \n\f\r\n\vfoo foo\t  \n\f\r\n\v";
  abel::trim_all(&str);
  EXPECT_EQ("foo foo", str);

  str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
  abel::trim_all(&str);
  EXPECT_EQ(abel::string_view{}, str);
}

TEST(trim_complete, InPlace) {
  const char* inputs[] = {"No extra space",
                          "  Leading whitespace",
                          "Trailing whitespace  ",
                          "  Leading and trailing  ",
                          " Whitespace \t  in\v   middle  ",
                          "'Eeeeep!  \n Newlines!\n",
                          "nospaces",
                          "",
                          "\n\t a\t\n\nb \t\n"};

  const char* outputs[] = {
      "No extra space",
      "Leading whitespace",
      "Trailing whitespace",
      "Leading and trailing",
      "Whitespace in middle",
      "'Eeeeep! Newlines!",
      "nospaces",
      "",
      "a\nb",
  };
  const int NUM_TESTS = ABEL_ARRAYSIZE(inputs);

  for (int i = 0; i < NUM_TESTS; i++) {
    std::string s(inputs[i]);
    abel::trim_complete(&s);
    EXPECT_EQ(outputs[i], s);
  }
}

}  // namespace
