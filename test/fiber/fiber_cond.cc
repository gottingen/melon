//
// Created by liyinbin on 2021/4/18.
//


#include "abel/fiber/fiber_cond.h"

#include <atomic>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "abel/base/random.h"
#include "abel/fiber/fiber.h"
#include "abel/fiber/fiber_mutex.h"
#include "abel/fiber/this_fiber.h"
#include "testing/fiber.h"


namespace abel {

    TEST(ConditionVariable, All) {
        testing::run_as_fiber([] {
            for (int k = 0; k != 10; ++k) {
                constexpr auto N = 600;

                std::atomic<std::size_t> run{};
                fiber_mutex lock[N];
                fiber_cond cv[N];
                bool set[N];
                std::vector<fiber> prod(N);
                std::vector<fiber> cons(N);

                for (int i = 0; i != N; ++i) {
                    prod[i] = fiber([&run, i, &cv, &lock, &set] {
                        fiber_sleep_for(Random(20) * abel::duration::milliseconds(1));
                        std::unique_lock lk(lock[i]);
                        cv[i].wait(lk, [&] { return set[i]; });
                        ++run;
                    });
                    cons[i] = fiber([&run, i, &cv, &lock, &set] {
                        fiber_sleep_for(Random(20) *  abel::duration::milliseconds(1));
                        std::scoped_lock _(lock[i]);
                        set[i] = true;
                        cv[i].notify_one();
                        ++run;
                    });
                }

                for (auto &&e : prod) {
                    ASSERT_TRUE(e.joinable());
                    e.join();
                }
                for (auto &&e : cons) {
                    ASSERT_TRUE(e.joinable());
                    e.join();
                }

                ASSERT_EQ(N * 2, run);
            }
        });
    }

}  // namespace abel