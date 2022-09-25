
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "melon/strings/compare.h"
#include "testing/gtest_wrap.h"

namespace {

    TEST(MatchTest, EqualsIgnoreCase) {
        std::string text = "the";
        std::string_view data(text);

        EXPECT_TRUE(melon::equal_case(data, "The"));
        EXPECT_TRUE(melon::equal_case(data, "THE"));
        EXPECT_TRUE(melon::equal_case(data, "the"));
        EXPECT_FALSE(melon::equal_case(data, "Quick"));
        EXPECT_FALSE(melon::equal_case(data, "then"));
    }

}
