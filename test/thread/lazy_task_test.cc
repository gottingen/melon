//
// Created by liyinbin on 2021/4/5.
//


#include "abel/thread/lazy_task.h"
#include "abel/chrono/clock.h"
#include <chrono>
#include <thread>

#include "gtest/gtest.h"

namespace abel {

    TEST(lazy_task, All) {
        int x = 0;

        auto id = set_thread_lazy_task([&] { ++x; }, abel::duration::milliseconds(1));

        abel::sleep_for(abel::duration::milliseconds(100));
        std::thread([] { notify_thread_lazy_task(); }).join();
        EXPECT_EQ(1, x);  // Every Thread Matters.

        notify_thread_lazy_task();
        EXPECT_EQ(2, x);  // Callback fired.
        notify_thread_lazy_task();
        EXPECT_EQ(2, x);  // Rate-throttled.

        abel::sleep_for(abel::duration::milliseconds(100));
        notify_thread_lazy_task();
        EXPECT_EQ(3, x);  // Fired again.

        delete_thread_lazy_task(id);
        abel::sleep_for(abel::duration::milliseconds(100));
        notify_thread_lazy_task();
        EXPECT_EQ(3, x);
    }

}  // namespace abel