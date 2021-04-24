//
// Created by liyinbin on 2021/4/16.
//


#include "abel/fiber/async.h"

#include "gtest/gtest.h"

#include "abel/future/future.h"
#include "testing/fiber.h"
#include "abel/fiber/future.h"
#include "abel/fiber/this_fiber.h"

using namespace std::literals;

namespace abel {

    TEST(Async, Execute) {
        testing::RunAsFiber([] {
            for (int i = 0; i != 10000; ++i) {
                int rc = 0;
                auto tid = std::this_thread::get_id();
                future<> ff = fiber_async(fiber_internal::Launch::Dispatch, [&] {
                    rc = 1;
                    ASSERT_EQ(tid, std::this_thread::get_id())<<"tid: "<<tid<<"std::this_thread::get_id(): "<<std::this_thread::get_id();
                });
                fiber_blocking_get(&ff);
                ASSERT_EQ(1, rc);
                future<int> f = fiber_async([&] {
                    // Which thread is running this abel is unknown. No assertion here.
                    return 5;
                });
                fiber_yield();
                ASSERT_EQ(5, fiber_blocking_get(&f));
            }
        });
    }

}  // namespace abel
