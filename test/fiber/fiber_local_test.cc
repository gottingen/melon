//
// Created by liyinbin on 2021/4/18.
//


#include "abel/fiber/fiber_local.h"

#include <atomic>
#include <thread>
#include <vector>

#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"

#include "abel/base/random.h"
#include "testing/fiber.h"
#include "abel/fiber/fiber.h"
#include "abel/fiber/this_fiber.h"

namespace abel {

    TEST(fiber_local, All) {
        for (int k = 0; k != 10; ++k) {
            testing::RunAsFiber([] {
                static fiber_local<int> fls;
                static fiber_local<int> fls2;
                static fiber_local<double> fls3;
                static fiber_local <std::vector<int>> fls4;
                constexpr auto N = 10000;

                std::atomic<std::size_t> run{};
                std::vector<fiber> fs(N);

                for (int i = 0; i != N; ++i) {
                    fs[i] = fiber([i, &run] {
                        *fls = i;
                        *fls2 = i * 2;
                        *fls3 = i + 3;
                        fls4->push_back(123);
                        fls4->push_back(456);
                        while (Random(20) > 1) {
                            fiber_sleep_for(Random(1000) * abel::duration::microseconds(1));
                            ASSERT_EQ(i, *fls);
                            ASSERT_EQ(i * 2, *fls2);
                            ASSERT_EQ(i + 3, *fls3);
                            ASSERT_THAT(*fls4, ::testing::ElementsAre(123, 456));
                        }
                        ++run;
                    });
                }

                for (auto &&e : fs) {
                    ASSERT_TRUE(e.joinable());
                    e.join();
                }

                ASSERT_EQ(N, run);
            });
        }
    }

}  // namespace abel
