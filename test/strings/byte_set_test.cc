//
// Created by liyinbin on 2021/5/1.
//


#include "abel/strings/byte_set.h"

#include "gtest/gtest.h"

namespace abel {

    constexpr byte_set empty;
    constexpr byte_set digits("0123456789");
    constexpr byte_set uppers("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    constexpr byte_set lowers("abcdefghijklmnopqrstuvwxyz");
    constexpr byte_set alphas(uppers | lowers);
    constexpr byte_set alnums(alphas | digits);

    TEST(byte_set, Empty) {
        for (int i = 0; i < UCHAR_MAX; ++i) {
            EXPECT_FALSE(empty.contains(i));
        }
    }

    TEST(byte_set, InsertAndFind) {
        byte_set bs;
        EXPECT_FALSE(bs.contains('A'));
        bs.insert('A');
        EXPECT_TRUE(bs.contains('A'));
        for (int i = 0; i < UCHAR_MAX; ++i) {
            EXPECT_EQ(i >= 'A' && i <= 'Z', uppers.contains(i));
            EXPECT_EQ(i >= '0' && i <= '9', digits.contains(i));
        }
    }

    TEST(byte_set, CharPtr) {
        const char *s = "ABCD";
        byte_set bs(s);
        const char *const cs = "ABCD";
        byte_set cbs(cs);
        EXPECT_EQ(bs, cbs);
    }

    TEST(byte_set, Or) {
        EXPECT_EQ(alphas, uppers | lowers);
        EXPECT_EQ(alnums, alphas | digits);
    }

    TEST(byte_set, And) {
        EXPECT_EQ(empty, uppers & lowers);
        EXPECT_EQ(alnums, alphas | digits);
    }

    TEST(byte_set, OrEq) {
        byte_set bs(lowers);
        bs |= uppers;
        EXPECT_EQ(alphas, bs);
    }

    TEST(byte_set, AndEq) {
        byte_set bs(alnums);
        bs &= digits;
        EXPECT_EQ(digits, bs);
    }

}  // abel
