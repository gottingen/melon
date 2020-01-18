//

#include <abel/base/internal/bits.h>

#include <gtest/gtest.h>

namespace {

int CLZ64(uint64_t n) {
  int fast = abel::base_internal::CountLeadingZeros64(n);
  int slow = abel::base_internal::CountLeadingZeros64Slow(n);
  EXPECT_EQ(fast, slow) << n;
  return fast;
}

TEST(BitsTest, CountLeadingZeros64) {
  EXPECT_EQ(64, CLZ64(uint64_t{}));
  EXPECT_EQ(0, CLZ64(~uint64_t{}));

  for (int index = 0; index < 64; index++) {
    uint64_t x = static_cast<uint64_t>(1) << index;
    const auto cnt = 63 - index;
    ASSERT_EQ(cnt, CLZ64(x)) << index;
    ASSERT_EQ(cnt, CLZ64(x + x - 1)) << index;
  }
}

int CLZ32(uint32_t n) {
  int fast = abel::base_internal::CountLeadingZeros32(n);
  int slow = abel::base_internal::CountLeadingZeros32Slow(n);
  EXPECT_EQ(fast, slow) << n;
  return fast;
}

TEST(BitsTest, CountLeadingZeros32) {
  EXPECT_EQ(32, CLZ32(uint32_t{}));
  EXPECT_EQ(0, CLZ32(~uint32_t{}));

  for (int index = 0; index < 32; index++) {
    uint32_t x = static_cast<uint32_t>(1) << index;
    const auto cnt = 31 - index;
    ASSERT_EQ(cnt, CLZ32(x)) << index;
    ASSERT_EQ(cnt, CLZ32(x + x - 1)) << index;
    ASSERT_EQ(CLZ64(x), CLZ32(x) + 32);
  }
}

int CTZ64(uint64_t n) {
  int fast = abel::base_internal::CountTrailingZerosNonZero64(n);
  int slow = abel::base_internal::CountTrailingZerosNonZero64Slow(n);
  EXPECT_EQ(fast, slow) << n;
  return fast;
}

TEST(BitsTest, CountTrailingZerosNonZero64) {
  EXPECT_EQ(0, CTZ64(~uint64_t{}));

  for (int index = 0; index < 64; index++) {
    uint64_t x = static_cast<uint64_t>(1) << index;
    const auto cnt = index;
    ASSERT_EQ(cnt, CTZ64(x)) << index;
    ASSERT_EQ(cnt, CTZ64(~(x - 1))) << index;
  }
}

int CTZ32(uint32_t n) {
  int fast = abel::base_internal::CountTrailingZerosNonZero32(n);
  int slow = abel::base_internal::CountTrailingZerosNonZero32Slow(n);
  EXPECT_EQ(fast, slow) << n;
  return fast;
}

TEST(BitsTest, CountTrailingZerosNonZero32) {
  EXPECT_EQ(0, CTZ32(~uint32_t{}));

  for (int index = 0; index < 32; index++) {
    uint32_t x = static_cast<uint32_t>(1) << index;
    const auto cnt = index;
    ASSERT_EQ(cnt, CTZ32(x)) << index;
    ASSERT_EQ(cnt, CTZ32(~(x - 1))) << index;
  }
}


}  // namespace
