
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#include "melon/base/type_traits.h"
#include <string>
#include "testing/gtest_wrap.h"

TEST(base, type_traits) {
    EXPECT_FALSE(melon::has_mapped_type<int>::value);
    EXPECT_TRUE(
            (melon::has_mapped_type<std::map<int, int>>::value));
    EXPECT_FALSE(melon::has_value_type<int>::value);
    EXPECT_TRUE(
            (melon::has_value_type<std::map<int, int>>::value));
    EXPECT_FALSE(melon::has_const_iterator<int>::value);
    EXPECT_TRUE(
            (melon::has_const_iterator<std::map<int, int>>::value));
}