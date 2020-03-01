//
// Created by liyinbin on 2020/1/23.
//

#include <abel/strings/case_conv.h>
#include <abel/strings/ascii.h>
#include <abel/strings/string_view.h>
#include <cctype>
#include <clocale>
#include <cstring>
#include <string>
#include <gtest/gtest.h>
#include <abel/base/profile.h>


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
                   mutable_buf, abel::ascii::to_lower);
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
                   mutable_buf, abel::ascii::to_upper);
    EXPECT_STREQ("MUTABLE", mutable_buf);
}
