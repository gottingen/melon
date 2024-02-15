// Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved


#include <gtest/gtest.h>
#include "melon/raft/memory_log.h"

namespace melon::raft {
extern void global_init_once_or_die();
};

class MemStorageTest : public testing::Test {
protected:
    void SetUp() {
        system("rm -rf data");
        melon::raft::global_init_once_or_die();
    }
    void TearDown() {}
};

TEST_F(MemStorageTest, init) {
    melon::raft::LogStorage* log_storage = melon::raft::LogStorage::create("memory://data/log");
    ASSERT_TRUE(log_storage);
    melon::raft::ConfigurationManager cm;
    ASSERT_EQ(0, log_storage->init(&cm));
    ASSERT_FALSE(melon::raft::LogStorage::create("hdfs://data/log"));
    ASSERT_FALSE(melon::raft::LogStorage::create("://data/log"));
    ASSERT_FALSE(melon::raft::LogStorage::create("data/log"));
    ASSERT_FALSE(melon::raft::LogStorage::create("  ://data/log"));
}

TEST_F(MemStorageTest, entry_operation) {
    melon::raft::LogStorage* log_storage = melon::raft::LogStorage::create("memory://data/log");
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

    ASSERT_EQ(1, log_storage->first_log_index());
    ASSERT_EQ(1, log_storage->last_log_index());
    entry = log_storage->get_entry(1);
    ASSERT_TRUE(entry);
    ASSERT_EQ("hello world", entry->data.to_string());
    ASSERT_EQ(melon::raft::LogId(1, 1), entry->id);
    int64_t term = log_storage->get_term(1);
    ASSERT_EQ(1, term);
    entry->Release();
    entry = NULL;

    ASSERT_EQ(0, log_storage->reset(10));
    ASSERT_EQ(10, log_storage->first_log_index());
    ASSERT_EQ(9, log_storage->last_log_index());
    delete log_storage;
}

TEST_F(MemStorageTest, trunk_operation) {
    melon::raft::LogStorage* log_storage = melon::raft::LogStorage::create("memory://data/log");
    ASSERT_TRUE(log_storage);
    melon::raft::ConfigurationManager cm;
    ASSERT_EQ(0, log_storage->init(&cm));
    std::vector<melon::raft::LogEntry*> entries;
    melon::raft::LogEntry* entry = new melon::raft::LogEntry();
    entry->data.append("hello world");
    entry->id = melon::raft::LogId(1, 1);
    entry->type = melon::raft::ENTRY_TYPE_DATA;
    entries.push_back(entry);
    melon::raft::LogEntry* entry1 = new melon::raft::LogEntry();
    entry1->data.append("hello world");
    entry1->id = melon::raft::LogId(2, 1);
    entry1->type = melon::raft::ENTRY_TYPE_DATA;
    entries.push_back(entry1);
    melon::raft::LogEntry* entry2 = new melon::raft::LogEntry();
    entry2->data.append("hello world");
    entry2->id = melon::raft::LogId(3, 1);
    entry2->type = melon::raft::ENTRY_TYPE_DATA;
    entries.push_back(entry2);
    ASSERT_EQ(3u, log_storage->append_entries(entries, NULL));

    size_t ret = log_storage->truncate_suffix(2);
    ASSERT_EQ(0, ret);
    ASSERT_EQ(1, log_storage->first_log_index());
    ASSERT_EQ(2, log_storage->last_log_index());

    ret = log_storage->truncate_prefix(2);
    ASSERT_EQ(0, ret);
    ASSERT_EQ(2, log_storage->first_log_index());
    ASSERT_EQ(2, log_storage->last_log_index());
    entry1->Release();
    delete log_storage;
}
