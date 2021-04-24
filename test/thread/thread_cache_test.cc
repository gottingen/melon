//
// Created by liyinbin on 2021/4/5.
//


#include "abel/thread/thread_cache.h"

#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "abel/strings/numbers.h"
#include "abel/chrono/clock.h"
#include "abel/thread/latch.h"
#include "abel/base/random.h"

using namespace std::literals;

namespace abel {

    TEST(thread_cache, Basic) {
        thread_cache<std::string> tc_str("123");
        for (int i = 0; i != 1000; ++i) {
            latch latch1(1), latch2(1);
            auto t = std::thread([&] {
                ASSERT_EQ("123", tc_str.non_idempotent_get());
                latch1.count_down();
                latch2.wait();
                ASSERT_EQ("456", tc_str.non_idempotent_get());
            });
            latch1.wait();
            tc_str.emplace("456");
            latch2.count_down();
            t.join();
            tc_str.emplace("123");
        }

// Were `thread_local` keyword used internally, the assertion below would
// fail.
        thread_cache<std::string> tc_str2("777");
        std::thread([&] { ASSERT_EQ("777", tc_str2.non_idempotent_get()); }).join();
    }

    TEST(thread_cache, Torture) {
        thread_cache<std::string> str;
        std::vector<std::thread> ts;

        for (int i = 0; i != 100; ++i) {
            ts.push_back(std::thread([&, s = abel::now()] {
                while (abel::now() + abel::duration::seconds(10) < s) {
                    if (Random() % 1000 == 0) {
                        str.emplace(std::to_string(Random() % 33333));
                    } else {
                        int64_t opt;
                        auto r = abel::simple_atoi(str.non_idempotent_get(), &opt);
                        ASSERT_TRUE(r);
                        ASSERT_LT(opt, 33333);
                    }
                }
            }));
        }
        for (auto &&e : ts) {
            e.join();
        }
    }

}  // namespace abel
