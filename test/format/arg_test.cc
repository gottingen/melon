// Copyright 2017 The abel Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
#include <abel/format/internal/arg.h>
#include <ostream>
#include <string>
#include <gtest/gtest.h>
#include <abel/format/str_format.h>

namespace abel {

namespace format_internal {
namespace {

class FormatArgImplTest : public ::testing::Test {
public:
    enum Color { kRed, kGreen, kBlue };

    static const char *hi () { return "hi"; }
};

TEST_F(FormatArgImplTest, ToInt) {
    int out = 0;
    EXPECT_TRUE(format_arg_impl_friend::to_int(format_arg_impl(1), &out));
    EXPECT_EQ(1, out);
    EXPECT_TRUE(format_arg_impl_friend::to_int(format_arg_impl(-1), &out));
    EXPECT_EQ(-1, out);
    EXPECT_TRUE(
        format_arg_impl_friend::to_int(format_arg_impl(static_cast<char>(64)), &out));
    EXPECT_EQ(64, out);
    EXPECT_TRUE(format_arg_impl_friend::to_int(
        format_arg_impl(static_cast<unsigned long long>(123456)), &out));  // NOLINT
    EXPECT_EQ(123456, out);
    EXPECT_TRUE(format_arg_impl_friend::to_int(
        format_arg_impl(static_cast<unsigned long long>(  // NOLINT
                          std::numeric_limits<int>::max()) +
            1),
        &out));
    EXPECT_EQ(std::numeric_limits<int>::max(), out);
    EXPECT_TRUE(format_arg_impl_friend::to_int(
        format_arg_impl(static_cast<long long>(  // NOLINT
                          std::numeric_limits<int>::min()) -
            10),
        &out));
    EXPECT_EQ(std::numeric_limits<int>::min(), out);
    EXPECT_TRUE(format_arg_impl_friend::to_int(format_arg_impl(false), &out));
    EXPECT_EQ(0, out);
    EXPECT_TRUE(format_arg_impl_friend::to_int(format_arg_impl(true), &out));
    EXPECT_EQ(1, out);
    EXPECT_FALSE(format_arg_impl_friend::to_int(format_arg_impl(2.2), &out));
    EXPECT_FALSE(format_arg_impl_friend::to_int(format_arg_impl(3.2f), &out));
    EXPECT_FALSE(format_arg_impl_friend::to_int(
        format_arg_impl(static_cast<int *>(nullptr)), &out));
    EXPECT_FALSE(format_arg_impl_friend::to_int(format_arg_impl(hi()), &out));
    EXPECT_FALSE(format_arg_impl_friend::to_int(format_arg_impl("hi"), &out));
    EXPECT_TRUE(format_arg_impl_friend::to_int(format_arg_impl(kBlue), &out));
    EXPECT_EQ(2, out);
}

extern const char kMyArray[];

TEST_F(FormatArgImplTest, CharArraysDecayToCharPtr) {
    const char *a = "";
    EXPECT_EQ(format_arg_impl_friend::GetVTablePtrForTest(format_arg_impl(a)),
              format_arg_impl_friend::GetVTablePtrForTest(format_arg_impl("")));
    EXPECT_EQ(format_arg_impl_friend::GetVTablePtrForTest(format_arg_impl(a)),
              format_arg_impl_friend::GetVTablePtrForTest(format_arg_impl("A")));
    EXPECT_EQ(format_arg_impl_friend::GetVTablePtrForTest(format_arg_impl(a)),
              format_arg_impl_friend::GetVTablePtrForTest(format_arg_impl("ABC")));
    EXPECT_EQ(format_arg_impl_friend::GetVTablePtrForTest(format_arg_impl(a)),
              format_arg_impl_friend::GetVTablePtrForTest(format_arg_impl(kMyArray)));
}

TEST_F(FormatArgImplTest, OtherPtrDecayToVoidPtr) {
    auto expected = format_arg_impl_friend::GetVTablePtrForTest(
        format_arg_impl(static_cast<void *>(nullptr)));
    EXPECT_EQ(format_arg_impl_friend::GetVTablePtrForTest(
        format_arg_impl(static_cast<int *>(nullptr))),
              expected);
    EXPECT_EQ(format_arg_impl_friend::GetVTablePtrForTest(
        format_arg_impl(static_cast<volatile int *>(nullptr))),
              expected);

    auto p = static_cast<void (*) ()>([] { });
    EXPECT_EQ(format_arg_impl_friend::GetVTablePtrForTest(format_arg_impl(p)),
              expected);
}

TEST_F(FormatArgImplTest, WorksWithCharArraysOfUnknownSize) {
    std::string s;
    format_sink_impl sink(&s);
    conversion_spec conv;
    conv.set_conv(conversion_char::FromChar('s'));
    conv.set_flags(format_flags());
    conv.set_width(-1);
    conv.set_precision(-1);
    EXPECT_TRUE(
        format_arg_impl_friend::convert(format_arg_impl(kMyArray), conv, &sink));
    sink.Flush();
    EXPECT_EQ("ABCDE", s);
}
const char kMyArray[] = "ABCDE";

}  // namespace
}  // namespace format_internal

}  // namespace abel
