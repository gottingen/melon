//

#include <abel/base/profile.h>

#include <cstdint>

#include <gtest/gtest.h>
#include <abel/synchronization/internal/thread_pool.h>

namespace {

TEST(ConfigTest, Endianness) {
    union {
        uint32_t value;
        uint8_t data[sizeof(uint32_t)];
    } number;
    number.data[0] = 0x00;
    number.data[1] = 0x01;
    number.data[2] = 0x02;
    number.data[3] = 0x03;
#if defined(ABEL_SYSTEM_LITTLE_ENDIAN) && defined(ABEL_SYSTEM_BIG_ENDIAN)
#error Both ABEL_SYSTEM_LITTLE_ENDIAN and ABEL_SYSTEM_BIG_ENDIAN are defined
#elif defined(ABEL_SYSTEM_LITTLE_ENDIAN)
    EXPECT_EQ(UINT32_C(0x03020100), number.value);
#elif defined(ABEL_SYSTEM_BIG_ENDIAN)
    EXPECT_EQ(UINT32_C(0x00010203), number.value);
#else
#error Unknown endianness
#endif
}

#if defined(ABEL_HAVE_THREAD_LOCAL)
TEST(ConfigTest, ThreadLocal) {
    static thread_local int mine_mine_mine = 16;
    EXPECT_EQ(16, mine_mine_mine);
    {
        abel::synchronization_internal::ThreadPool pool(1);
        pool.Schedule([&] {
            EXPECT_EQ(16, mine_mine_mine);
            mine_mine_mine = 32;
            EXPECT_EQ(32, mine_mine_mine);
        });
    }
    EXPECT_EQ(16, mine_mine_mine);
}
#endif

}  // namespace
