//
// Created by liyinbin on 2020/4/11.
//

#include <gtest/gtest.h>
#include <abel/config/flag.h>

ABEL_FLAG(bool, test_bool_1, false, "help 1");
ABEL_FLAG(bool, test_bool_2, true, "help 2");

TEST(flags, visitor) {

    auto cv = abel::get_all_flags();
    EXPECT_EQ(2, cv.size());
    EXPECT_EQ(FLAGS_test_bool_1.get(), cv[0]->get<bool>());
    EXPECT_EQ(FLAGS_test_bool_2.get(), cv[1]->get<bool>());
}
