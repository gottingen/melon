// Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved


#include <gtest/gtest.h>
#include "melon/raft/storage.h"

namespace melon::raft {
extern void global_init_once_or_die();
};

class StorageTest : public testing::Test {
protected:
    void SetUp() {
        system("rm -rf data");
        melon::raft::global_init_once_or_die();
    }
    void TearDown() {}
};

TEST_F(StorageTest, sanity) {
    // LogStorage
    melon::raft::LogStorage* log_storage = melon::raft::LogStorage::create("local://data/log");
    ASSERT_TRUE(log_storage);
    melon::raft::ConfigurationManager cm;
    ASSERT_EQ(0, log_storage->init(&cm));
    ASSERT_FALSE(melon::raft::LogStorage::create("hdfs://data/log"));
    ASSERT_FALSE(melon::raft::LogStorage::create("://data/log"));
    ASSERT_FALSE(melon::raft::LogStorage::create("data/log"));
    ASSERT_FALSE(melon::raft::LogStorage::create("  ://data/log"));

    // RaftMetaStorage
    melon::raft::RaftMetaStorage* meta_storage
            = melon::raft::RaftMetaStorage::create("local://data/raft_meta");
    ASSERT_TRUE(meta_storage);
    ASSERT_TRUE(meta_storage->init().ok());
    ASSERT_FALSE(melon::raft::RaftMetaStorage::create("hdfs://data/raft_meta"));
    ASSERT_FALSE(melon::raft::RaftMetaStorage::create("://data/raft_meta"));
    ASSERT_FALSE(melon::raft::RaftMetaStorage::create("data/raft_meta"));
    ASSERT_FALSE(melon::raft::RaftMetaStorage::create("  ://data/raft_meta"));

    // SnapshotStorage
    melon::raft::SnapshotStorage* snapshot_storage
            = melon::raft::SnapshotStorage::create("local://data/snapshot");
    ASSERT_TRUE(snapshot_storage);
    ASSERT_EQ(0, snapshot_storage->init());
    ASSERT_FALSE(melon::raft::SnapshotStorage::create("hdfs://data/snapshot"));
    ASSERT_FALSE(melon::raft::SnapshotStorage::create("://data/snapshot"));
    ASSERT_FALSE(melon::raft::SnapshotStorage::create("data/snapshot"));
    ASSERT_FALSE(melon::raft::SnapshotStorage::create("  ://data/snapshot"));

}

TEST_F(StorageTest, extra_space_should_be_trimmed) {
    // LogStorage
    melon::raft::LogStorage* log_storage = melon::raft::LogStorage::create("local://data/log");
    ASSERT_TRUE(log_storage);
    melon::raft::ConfigurationManager cm;
    ASSERT_EQ(0, log_storage->init(&cm));
    melon::raft::LogEntry* entry = new melon::raft::LogEntry();
    entry->data.append("hello world");
    entry->id = melon::raft::LogId(1, 1);
    entry->type = melon::raft::ENTRY_TYPE_DATA;
    std::vector<melon::raft::LogEntry*> entries;
    entries.push_back(entry);
    ASSERT_EQ(1u, log_storage->append_entries(entries, NULL));
    entry->Release();
    delete log_storage;

    // reopen
    log_storage = melon::raft::LogStorage::create(" local://./  data// // log ////");
    ASSERT_EQ(0, log_storage->init(&cm));
    
    ASSERT_EQ(1, log_storage->first_log_index());
    ASSERT_EQ(1, log_storage->last_log_index());
    entry = log_storage->get_entry(1);
    ASSERT_TRUE(entry);
    ASSERT_EQ("hello world", entry->data.to_string());
    ASSERT_EQ(melon::raft::LogId(1, 1), entry->id);
    entry->Release();
    entry = NULL;
}
