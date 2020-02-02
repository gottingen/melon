//

#include <abel/base/math/clz.h>
#include <abel/base/math/ctz.h>
#include <gtest/gtest.h>

namespace {

int CLZ64 (uint64_t n) {
    auto fast = abel::count_leading_zeros(n);
    auto slow = abel::clz_template(n);
    EXPECT_EQ(fast, slow) << n;
    return fast;
}

TEST(BitsTest, count_leading_zeros_64) {
    EXPECT_EQ(64, CLZ64(uint64_t {}));
    EXPECT_EQ(0, CLZ64(~uint64_t {}));

    for (int index = 0; index < 64; index++) {
        uint64_t x = static_cast<uint64_t>(1) << index;
        const auto cnt = 63 - index;
        ASSERT_EQ(cnt, CLZ64(x)) << index;
        ASSERT_EQ(cnt, CLZ64(x + x - 1)) << index;
    }
}

int CLZ32 (uint32_t n) {
    auto fast = abel::count_leading_zeros(n);
    auto slow = abel::clz_template(n);
    EXPECT_EQ(fast, slow) << n;
    return fast;
}

TEST(BitsTest, count_leading_zeros_32) {
    EXPECT_EQ(32, CLZ32(uint32_t {}));
    EXPECT_EQ(0, CLZ32(~uint32_t {}));

    for (int index = 0; index < 32; index++) {
        uint32_t x = static_cast<uint32_t>(1) << index;
        const auto cnt = 31 - index;
        ASSERT_EQ(cnt, CLZ32(x)) << index;
        ASSERT_EQ(cnt, CLZ32(x + x - 1)) << index;
        ASSERT_EQ(CLZ64(x), CLZ32(x) + 32);
    }
}

int CTZ64 (uint64_t n) {
    auto fast = abel::count_trailing_zeros(n);
    auto slow = abel::ctz_template(n);
    EXPECT_EQ(fast, slow) << n;
    return fast;
}

TEST(BitsTest, count_tailing_zeros_64) {
    EXPECT_EQ(0, CTZ64(~uint64_t {}));

    for (int index = 0; index < 64; index++) {
        uint64_t x = static_cast<uint64_t>(1) << index;
        const auto cnt = index;
        ASSERT_EQ(cnt, CTZ64(x)) << index;
        ASSERT_EQ(cnt, CTZ64(~(x - 1))) << index;
    }
}

int CTZ32 (uint32_t n) {
    auto fast = abel::count_trailing_zeros(n);
    auto slow = abel::ctz_template(n);
    EXPECT_EQ(fast, slow) << n;
    return fast;
}

TEST(BitsTest, count_tailing_zeros_32) {
    EXPECT_EQ(0, CTZ32(~uint32_t {}));

    for (int index = 0; index < 32; index++) {
        uint32_t x = static_cast<uint32_t>(1) << index;
        const auto cnt = index;
        ASSERT_EQ(cnt, CTZ32(x)) << index;
        ASSERT_EQ(cnt, CTZ32(~(x - 1))) << index;
    }
}

}  // namespace
