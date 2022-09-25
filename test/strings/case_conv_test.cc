
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "melon/strings/case_conv.h"
#include "melon/strings/ascii.h"
#include <string_view>
#include <cctype>
#include <clocale>
#include <cstring>
#include <string>
#include "testing/gtest_wrap.h"
#include "melon/base/profile.h"


TEST(AsciiStrTo, Lower) {
    const char buf[] = "ABCDEF";
    const std::string str("GHIJKL");
    const std::string str2("MNOPQR");
    const std::string_view sp(str2);

    EXPECT_EQ("abcdef", melon::string_to_lower(buf));
    EXPECT_EQ("ghijkl", melon::string_to_lower(str));
    EXPECT_EQ("mnopqr", melon::string_to_lower(sp));

    char mutable_buf[] = "Mutable";
    std::transform(mutable_buf, mutable_buf + strlen(mutable_buf),
                   mutable_buf, melon::ascii::to_lower);
    EXPECT_STREQ("mutable", mutable_buf);
}

TEST(AsciiStrTo, Upper) {
    const char buf[] = "abcdef";
    const std::string str("ghijkl");
    const std::string str2("mnopqr");
    const std::string_view sp(str2);

    EXPECT_EQ("ABCDEF", melon::string_to_upper(buf));
    EXPECT_EQ("GHIJKL", melon::string_to_upper(str));
    EXPECT_EQ("MNOPQR", melon::string_to_upper(sp));

    char mutable_buf[] = "Mutable";
    std::transform(mutable_buf, mutable_buf + strlen(mutable_buf),
                   mutable_buf, melon::ascii::to_upper);
    EXPECT_STREQ("MUTABLE", mutable_buf);
}
