//
// Created by liyinbin on 2020/1/23.
//

#include <abel/strings/trim.h>
#include <cctype>
#include <clocale>
#include <cstring>
#include <string>
#include <gtest/gtest.h>
#include <abel/base/profile.h>

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
    const char *inputs[] = {"No extra space",
                            "  Leading whitespace",
                            "Trailing whitespace  ",
                            "  Leading and trailing  ",
                            " Whitespace \t  in\v   middle  ",
                            "'Eeeeep!  \n Newlines!\n",
                            "nospaces",
                            "",
                            "\n\t a\t\n\nb \t\n"};

    const char *outputs[] = {
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