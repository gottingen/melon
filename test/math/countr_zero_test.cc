//
// Created by liyinbin on 2020/3/1.
//

#include <abel/math/countr_zero.h>
#include <gtest/gtest.h>

int CTZ64(uint64_t n) {
    auto fast = abel::countr_zero(n);
    auto slow = abel::countr_zero_template(n);
    EXPECT_EQ(fast, slow) << n;
    return fast;
}

int CTZ32(uint32_t n) {
    auto fast = abel::countr_zero(n);
    auto slow = abel::countr_zero_template(n);
    EXPECT_EQ(fast, slow) << n;
    return fast;
}

TEST(countr_zero, countr_zero_64) {
    EXPECT_EQ(0, CTZ64(~uint64_t{}));

    for (int index = 0; index < 64; index++) {
        uint64_t x = static_cast<uint64_t>(1) << index;
        const auto cnt = index;
        ASSERT_EQ(cnt, CTZ64(x)) << index;
        ASSERT_EQ(cnt, CTZ64(~(x - 1))) << index;
    }
}

TEST(countr_zero, countr_zero_32) {
    EXPECT_EQ(0, CTZ32(~uint32_t{}));

    for (int index = 0; index < 32; index++) {
        uint32_t x = static_cast<uint32_t>(1) << index;
        const auto cnt = index;
        ASSERT_EQ(cnt, CTZ32(x)) << index;
        ASSERT_EQ(cnt, CTZ32(~(x - 1))) << index;
    }
}
