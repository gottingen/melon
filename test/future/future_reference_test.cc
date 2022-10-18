
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <iostream>
#include <queue>
#include "testing/gtest_wrap.h"
#include "melon/future/future.h"
#include <array>
#include <queue>


TEST(future, future_of_reference) {
    melon::promise<std::reference_wrapper<int>> p;
    melon::future <std::reference_wrapper<int>> f = p.get_future();

    int var = 0;

    p.set_value(var);

    f.finally([](melon::expected <std::reference_wrapper<int>> dst) { (*dst).get() = 4; });

    ASSERT_EQ(var, 4);
}