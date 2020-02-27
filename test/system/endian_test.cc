//

#include <abel/system/endian.h>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>
#include <gtest/gtest.h>
#include <abel/base/profile.h>

namespace abel {

    namespace {

        const uint64_t kInitialNumber{0x0123456789abcdef};
        const uint64_t k64Value{kInitialNumber};
        const uint32_t k32Value{0x01234567};
        const uint16_t k16Value{0x0123};
        const int kNumValuesToTest = 1000000;
        const int kRandomSeed = 12345;

#if defined(ABEL_SYSTEM_BIG_ENDIAN)
        const uint64_t kInitialInNetworkOrder{kInitialNumber};
        const uint64_t k64ValueLE{0xefcdab8967452301};
        const uint32_t k32ValueLE{0x67452301};
        const uint16_t k16ValueLE{0x2301};

        const uint64_t k64ValueBE{kInitialNumber};
        const uint32_t k32ValueBE{k32Value};
        const uint16_t k16ValueBE{k16Value};
#elif defined(ABEL_SYSTEM_LITTLE_ENDIAN)
        const uint64_t kInitialInNetworkOrder{0xefcdab8967452301};
        const uint64_t k64ValueLE{kInitialNumber};
        const uint32_t k32ValueLE{k32Value};
        const uint16_t k16ValueLE{k16Value};

        const uint64_t k64ValueBE{0xefcdab8967452301};
        const uint32_t k32ValueBE{0x67452301};
        const uint16_t k16ValueBE{0x2301};
#endif

        template<typename T>
        std::vector<T> GenerateAllValuesForType() {
            std::vector<T> result;
            T next = std::numeric_limits<T>::min();
            while (true) {
                result.push_back(next);
                if (next == std::numeric_limits<T>::max()) {
                    return result;
                }
                ++next;
            }
        }

        template<typename T>
        std::vector<T> GenerateRandomIntegers(size_t numValuesToTest) {
            std::vector<T> result;
            std::mt19937_64 rng(kRandomSeed);
            for (size_t i = 0; i < numValuesToTest; ++i) {
                result.push_back(rng());
            }
            return result;
        }

        void ManualByteSwap(char *bytes, int length) {
            if (length == 1)
                return;

            EXPECT_EQ(0, length % 2);
            for (int i = 0; i < length / 2; ++i) {
                int j = (length - 1) - i;
                using std::swap;
                swap(bytes[i], bytes[j]);
            }
        }

        template<typename T>
        ABEL_FORCE_INLINE T Unalignedload(const char *p) {
            static_assert(
                    sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
                    "Unexpected type size");

            switch (sizeof(T)) {
                case 1:
                    return *reinterpret_cast<const T *>(p);
                case 2:
                    return ABEL_INTERNAL_UNALIGNED_LOAD16(p);
                case 4:
                    return ABEL_INTERNAL_UNALIGNED_LOAD32(p);
                case 8:
                    return ABEL_INTERNAL_UNALIGNED_LOAD64(p);
                default:
                    // Suppresses invalid "not all control paths return a value" on MSVC
                    return {};
            }
        }

        template<typename T, typename ByteSwapper>
        static void GBSwapHelper(const std::vector<T> &host_values_to_test,
                                 const ByteSwapper &byte_swapper) {
            // Test byte_swapper against a manual byte swap.
            for (typename std::vector<T>::const_iterator it = host_values_to_test.begin();
                 it != host_values_to_test.end(); ++it) {
                T host_value = *it;

                char actual_value[sizeof(host_value)];
                memcpy(actual_value, &host_value, sizeof(host_value));
                byte_swapper(actual_value);

                char expected_value[sizeof(host_value)];
                memcpy(expected_value, &host_value, sizeof(host_value));
                ManualByteSwap(expected_value, sizeof(host_value));

                ASSERT_EQ(0, memcmp(actual_value, expected_value, sizeof(host_value)))
                                            << "Swap output for 0x" << std::hex << host_value << " does not match. "
                                            << "Expected: 0x" << Unalignedload<T>(expected_value) << "; "
                                            << "actual: 0x" << Unalignedload<T>(actual_value);
            }
        }

        void Swap16(char *bytes) {
            ABEL_INTERNAL_UNALIGNED_STORE16(
                    bytes, bit_swap16(ABEL_INTERNAL_UNALIGNED_LOAD16(bytes)));
        }

        void Swap32(char *bytes) {
            ABEL_INTERNAL_UNALIGNED_STORE32(
                    bytes, bit_swap32(ABEL_INTERNAL_UNALIGNED_LOAD32(bytes)));
        }

        void Swap64(char *bytes) {
            ABEL_INTERNAL_UNALIGNED_STORE64(
                    bytes, bit_swap64(ABEL_INTERNAL_UNALIGNED_LOAD64(bytes)));
        }

        TEST(EndianessTest, Uint16) {
            GBSwapHelper(GenerateAllValuesForType<uint16_t>(), &Swap16);
        }

        TEST(EndianessTest, Uint32) {
            GBSwapHelper(GenerateRandomIntegers<uint32_t>(kNumValuesToTest), &Swap32);
        }

        TEST(EndianessTest, Uint64) {
            GBSwapHelper(GenerateRandomIntegers<uint64_t>(kNumValuesToTest), &Swap64);
        }

        TEST(EndianessTest, ghtonll_gntohll) {
            // Test that abel::ghtonl compiles correctly
            uint32_t test = 0x01234567;
            EXPECT_EQ(abel::abel_ntohl(abel::abel_htonl(test)), test);

            uint64_t comp = abel::abel_htonll(kInitialNumber);
            EXPECT_EQ(comp, kInitialInNetworkOrder);
            comp = abel::abel_ntohll(kInitialInNetworkOrder);
            EXPECT_EQ(comp, kInitialNumber);

            // Test that htonll and ntohll are each others' inverse functions on a
            // somewhat assorted batch of numbers. 37 is chosen to not be anything
            // particularly nice base 2.
            uint64_t value = 1;
            for (int i = 0; i < 100; ++i) {
                comp = abel::abel_htonll(abel::abel_ntohll(value));
                EXPECT_EQ(value, comp);
                comp = abel::abel_ntohll(abel::abel_htonll(value));
                EXPECT_EQ(value, comp);
                value *= 37;
            }
        }

        TEST(EndianessTest, little_endian) {
            // Check little_endian uint16_t.
            uint64_t comp = little_endian::from_host16(k16Value);
            EXPECT_EQ(comp, k16ValueLE);
            comp = little_endian::to_host16(k16ValueLE);
            EXPECT_EQ(comp, k16Value);

            // Check little_endian uint32_t.
            comp = little_endian::from_host32(k32Value);
            EXPECT_EQ(comp, k32ValueLE);
            comp = little_endian::to_host32(k32ValueLE);
            EXPECT_EQ(comp, k32Value);

            // Check little_endian uint64_t.
            comp = little_endian::from_host64(k64Value);
            EXPECT_EQ(comp, k64ValueLE);
            comp = little_endian::to_host64(k64ValueLE);
            EXPECT_EQ(comp, k64Value);

            // Check little-endian load and store functions.
            uint16_t u16Buf;
            uint32_t u32Buf;
            uint64_t u64Buf;

            little_endian::store16(&u16Buf, k16Value);
            EXPECT_EQ(u16Buf, k16ValueLE);
            comp = little_endian::load16(&u16Buf);
            EXPECT_EQ(comp, k16Value);

            little_endian::store32(&u32Buf, k32Value);
            EXPECT_EQ(u32Buf, k32ValueLE);
            comp = little_endian::load32(&u32Buf);
            EXPECT_EQ(comp, k32Value);

            little_endian::store64(&u64Buf, k64Value);
            EXPECT_EQ(u64Buf, k64ValueLE);
            comp = little_endian::load64(&u64Buf);
            EXPECT_EQ(comp, k64Value);
        }

        TEST(EndianessTest, big_endian) {
            // Check big-endian load and store functions.
            uint16_t u16Buf;
            uint32_t u32Buf;
            uint64_t u64Buf;

            unsigned char buffer[10];
            big_endian::store16(&u16Buf, k16Value);
            EXPECT_EQ(u16Buf, k16ValueBE);
            uint64_t comp = big_endian::load16(&u16Buf);
            EXPECT_EQ(comp, k16Value);

            big_endian::store32(&u32Buf, k32Value);
            EXPECT_EQ(u32Buf, k32ValueBE);
            comp = big_endian::load32(&u32Buf);
            EXPECT_EQ(comp, k32Value);

            big_endian::store64(&u64Buf, k64Value);
            EXPECT_EQ(u64Buf, k64ValueBE);
            comp = big_endian::load64(&u64Buf);
            EXPECT_EQ(comp, k64Value);

            big_endian::store16(buffer + 1, k16Value);
            EXPECT_EQ(u16Buf, k16ValueBE);
            comp = big_endian::load16(buffer + 1);
            EXPECT_EQ(comp, k16Value);

            big_endian::store32(buffer + 1, k32Value);
            EXPECT_EQ(u32Buf, k32ValueBE);
            comp = big_endian::load32(buffer + 1);
            EXPECT_EQ(comp, k32Value);

            big_endian::store64(buffer + 1, k64Value);
            EXPECT_EQ(u64Buf, k64ValueBE);
            comp = big_endian::load64(buffer + 1);
            EXPECT_EQ(comp, k64Value);
        }

    }  // namespace

}  // namespace abel
