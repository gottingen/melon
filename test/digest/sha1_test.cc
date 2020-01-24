//
// Created by liyinbin on 2020/1/25.
//
#include <abel/digest/sha1.h>
#include <gtest/gtest.h>

TEST(Sha1, all) {
    EXPECT_EQ(
        abel::sha1_hex(""),
        "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    EXPECT_EQ(
        abel::sha1_hex("abc"),
        "a9993e364706816aba3e25717850c26c9cd0d89d");
    EXPECT_EQ(
        abel::sha1_hex("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
        "761c457bf73b14d27e9e9265c46f4b4dda11f940");
    EXPECT_EQ(
        abel::sha1_hex("12345678901234567890123456789012345678901234567890123456789012345678901234567890"),
        "50abf5706a150990a08b2c5ea40fa0e585554732");
}
