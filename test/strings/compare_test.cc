// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/strings/compare.h"
#include "gtest/gtest.h"

namespace {

    TEST(MatchTest, EqualsIgnoreCase) {
        std::string text = "the";
        std::string_view data(text);

        EXPECT_TRUE(abel::equal_case(data, "The"));
        EXPECT_TRUE(abel::equal_case(data, "THE"));
        EXPECT_TRUE(abel::equal_case(data, "the"));
        EXPECT_FALSE(abel::equal_case(data, "Quick"));
        EXPECT_FALSE(abel::equal_case(data, "then"));
    }

}
