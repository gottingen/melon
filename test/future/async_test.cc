
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <queue>
#include "testing/gtest_wrap.h"
#include "melon/future/future.h"

TEST(future, async) {
    std::queue<std::function<void()>> queue;

    auto fut = melon::async(queue, []() { return 12; });

    EXPECT_EQ(1, queue.size());
    int dst = 0;
    fut.finally([&](melon::expected<int, std::exception_ptr> x) { dst = x.value(); });
    EXPECT_EQ(0, dst);

    queue.front()();
    queue.pop();

    EXPECT_EQ(12, dst);
}