/*
 * =====================================================================================
 *
 *       Filename:  test_log_entry.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015年11月27日 11时11分35秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *
 * =====================================================================================
 */

#include <gtest/gtest.h>
#include <turbo/log/logging.h>
#include <melon/base/iobuf.h>
#include "melon/raft/log_entry.h"

class TestUsageSuits : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(TestUsageSuits, LogEntry) {
    melon::raft::LogEntry* entry = new melon::raft::LogEntry();
    std::vector<melon::raft::PeerId> peers;
    peers.push_back(melon::raft::PeerId("1.2.3.4:1000"));
    peers.push_back(melon::raft::PeerId("1.2.3.4:2000"));
    peers.push_back(melon::raft::PeerId("1.2.3.4:3000"));
    entry->type = melon::raft::ENTRY_TYPE_CONFIGURATION;
    entry->peers = new std::vector<melon::raft::PeerId>(peers);

    entry->AddRef();
    entry->Release();
    entry->Release();

    entry = new melon::raft::LogEntry();
    entry->type = melon::raft::ENTRY_TYPE_DATA;
    mutil::IOBuf buf;
    buf.append("hello, world");
    entry->data = buf;
    entry->data = buf;

    entry->Release();
}
