//
// Created by liyinbin on 2020/1/25.
//

#include <abel/digest/md5.h>
#include <gtest/gtest.h>

TEST (md5, all) {
    EXPECT_EQ(abel::md5_hex(""),
              "d41d8cd98f00b204e9800998ecf8427e");
    EXPECT_EQ(abel::md5_hex("a"),
              "0cc175b9c0f1b6a831c399e269772661");
    EXPECT_EQ(abel::md5_hex("abc"),
              "900150983cd24fb0d6963f7d28e17f72");
    EXPECT_EQ(
        abel::md5_hex("message digest"),
        "f96b697d7cb7938d525a2f31aaf161d0");
    EXPECT_EQ(
        abel::md5_hex("abcdefghijklmnopqrstuvwxyz"),
        "c3fcd3d76192e4007dfb496cca67e13b");
    EXPECT_EQ(
        abel::md5_hex("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
        "d174ab98d277d9f5a5611c2c9f419d9f");
    EXPECT_EQ(
        abel::md5_hex("12345678901234567890123456789012345678901234567890123456789012345678901234567890"),
        "57edf4a22be3c955ac49da2e2107b67a");
}