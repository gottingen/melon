/*
 * =====================================================================================
 *
 *       Filename:  test_protobuf_file.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015/09/22 19:48:31
 *       Revision:  none
 *       Compiler:  gcc
 *
 *
 * =====================================================================================
 */

#include <gtest/gtest.h>
#include "melon/proto/raft/local_storage.pb.h"
#include "melon/raft/protobuf_file.h"

class TestUsageSuits : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(TestUsageSuits, protobuf_file) {
    int ret = 0;

    melon::raft::ProtoBufFile pb_file("./log.meta");
    melon::raft::LogPBMeta meta;
    meta.set_first_log_index(1234);

    ret = pb_file.save(static_cast<google::protobuf::Message*>(&meta), false);
    ASSERT_EQ(ret, 0);

    {
        melon::raft::LogPBMeta new_meta;
        ret = pb_file.load(&new_meta);
        ASSERT_EQ(ret, 0);

        ASSERT_EQ(new_meta.first_log_index(), 1234);
    }

    ret = pb_file.save(&meta, true);
    ASSERT_EQ(ret, 0);

    {
        melon::raft::LogPBMeta new_meta;
        ret = pb_file.load(&new_meta);
        ASSERT_EQ(ret, 0);

        ASSERT_EQ(new_meta.first_log_index(), 1234);

        new_meta.PrintDebugString();
    }
}
