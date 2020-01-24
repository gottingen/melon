//
// Created by liyinbin on 2020/1/24.
//

#include <abel/strings/compare.h>
#include <gtest/gtest.h>

namespace {

TEST(MatchTest, EqualsIgnoreCase) {
    std::string text = "the";
    abel::string_view data(text);

    EXPECT_TRUE(abel::equal_case(data, "The"));
    EXPECT_TRUE(abel::equal_case(data, "THE"));
    EXPECT_TRUE(abel::equal_case(data, "the"));
    EXPECT_FALSE(abel::equal_case(data, "Quick"));
    EXPECT_FALSE(abel::equal_case(data, "then"));
}

}