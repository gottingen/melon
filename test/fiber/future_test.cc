//
// Created by liyinbin on 2021/4/18.
//


#include "abel/fiber/future.h"

#include <chrono>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "abel/base/random.h"
#include "abel/fiber/async.h"
#include "testing/fiber.h"
#include "abel/fiber/runtime.h"
#include "abel/fiber/this_fiber.h"

using namespace std::literals;

namespace abel {

    TEST(Future, BlockingGet) {
        testing::RunAsFiber([&] {
            for (int i = 0; i != 200; ++i) {
                fiber fbs[100];
                for (auto &&f : fbs) {
                    auto op = [] {
                        auto v = fiber_blocking_get(fiber_async([] {
                            std::vector<int> v{1, 2, 3, 4, 5};
                            int round = Random(10);
                            for (int j = 0; j != round; ++j) {
                                fiber_yield();
                            }
                            return v;
                        }));
                        ASSERT_THAT(v, ::testing::ElementsAre(1, 2, 3, 4, 5));
                    };
                    auto sg = Random() % get_scheduling_group_count();
                    f = fiber(fiber::attributes{.scheduling_group = sg}, op);
                }
                for (auto &&f : fbs) {
                    f.join();
                }
            }
        });
    }

    TEST(Future, BlockingTryGetOk) {
        testing::RunAsFiber([&] {
            std::atomic<bool> f{};
            auto future = fiber_async([&] {
                fiber_sleep_for(abel::duration::seconds(1));
                f = true;
            });
            ASSERT_FALSE(fiber_blocking_try_get(std::move(future), abel::duration::milliseconds(10)));
            ASSERT_FALSE(f);
            fiber_sleep_for(abel::duration::seconds(2));
            ASSERT_TRUE(f);
        });
    }

    TEST(Future, BlockingTryGetTimeout) {
        testing::RunAsFiber([&] {
            auto future = fiber_async([] {
                fiber_sleep_for(abel::duration::seconds(1));
                return 10;
            });
            auto result = fiber_blocking_try_get(std::move(future), abel::duration::seconds(2));
            ASSERT_TRUE(result);
            EXPECT_EQ(10, *result);
        });
    }

}  // namespace abel
