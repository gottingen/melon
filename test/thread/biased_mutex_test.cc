//
// Created by liyinbin on 2021/4/5.
//


#include "abel/thread/biased_mutex.h"

#include <chrono>
#include <thread>

#include "gtest/gtest.h"



namespace abel {

    TEST(biased_mutex, All) {
    struct alignas(128) AlignedInt {
        std::int64_t v;
    };
    AlignedInt separated_counters[3] = {};
    std::atomic<bool> leave{false};

    biased_mutex biased_mutex;
    // Both protected by `biased_mutex`.
    std::int64_t v = 0;
    std::int64_t v_copy;

    std::thread blessed([&] {
        while (!leave.load(std::memory_order_relaxed)) {
            ++separated_counters[0].v;
            std::scoped_lock _(*biased_mutex.get_blessed_side());
            ++v;
            v_copy = v;
        }
    });
    std::thread really_slow([&] {
        auto start = abel::now();
        while (abel::now() - start < abel::duration::seconds(10)) {
            ++separated_counters[1].v;
            std::scoped_lock _(*biased_mutex.get_really_slow_side());
            ++v;
            v_copy = v;
        }
    });
    std::thread really_slow2([&] {
        auto start = abel::now();
        while (abel::now() - start < abel::duration::seconds(10)) {
            ++separated_counters[2].v;
            std::scoped_lock _(*biased_mutex.get_really_slow_side());
            ++v;
            v_copy = v;
        }
    });

    really_slow.join();
    really_slow2.join();
    leave = true;
    blessed.join();

    std::scoped_lock _(*biased_mutex.get_really_slow_side());
    ASSERT_EQ(v, separated_counters[0].v + separated_counters[1].v +
    separated_counters[2].v);
    ASSERT_EQ(v, v_copy);
}

}  // namespace abel
