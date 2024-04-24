// Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved



#include <gtest/gtest.h>
#include <melon/raft/util.h>
#include <melon/utility/crc32c.h>

class ChecksumTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(ChecksumTest, benchmark) {
    char data[4096];
    for (size_t i = 0; i < ARRAY_SIZE(data); ++i) {
        data[i] = mutil::fast_rand_in('a', 'z');
    }
    mutil::Timer timer;
    const size_t N = 10000;
    timer.start();
    for (size_t i = 0; i < N; ++i) {
        melon::raft::murmurhash32(data, sizeof(data));
    }
    timer.stop();
    const long mur_elp = timer.u_elapsed();
    
    timer.start();
    for (size_t i = 0; i < N; ++i) {
        mutil::crc32c::Value(data, sizeof(data));
    }
    timer.stop();
    const long crc_elp = timer.u_elapsed();

    MLOG(INFO) << "murmurhash32_TP=" << sizeof(data) * N / (double)mur_elp << "MB/s"
              << " base_crc32_TP=" << sizeof(data) * N / (double)crc_elp << "MB/s";
    MLOG(INFO) << "base_is_fast_crc32_support=" << mutil::crc32c::IsFastCrc32Supported();

}
