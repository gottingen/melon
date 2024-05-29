// libraft - Quorum-based replication of states across machines.
// Copyright (c) 2015 Baidu.com, Inc. All Rights Reserved



#include <gtest/gtest.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <melon/utility/atomicops.h>
#include <melon/utility/file_util.h>
#include <melon/utility/files/file_path.h>
#include <melon/utility/files/file_enumerator.h>
#include <melon/utility/files/dir_reader_posix.h>
#include <melon/utility/string_printf.h>
#include <turbo/log/logging.h>
#include <melon/raft/util.h>
#include <melon/raft/log.h>
#include <melon/raft/storage.h>
#include <melon/raft/config.h>
#include <cinttypes>

namespace melon::raft {
DECLARE_bool(raft_trace_append_entry_latency);
}

class LogStorageTest : public testing::Test {
protected:
    void SetUp() {
        melon::raft::FLAGS_raft_sync = false;
        google::SetCommandLineOption("minloglevel", "3");
    }
    void TearDown() {}
};

TEST_F(LogStorageTest, open_segment) {
    // open segment operation
    ::system("mkdir data/");
    melon::raft::Segment* seg1 = new melon::raft::Segment("./data", 1L, 0);

    // not open
    melon::raft::LogEntry* entry = seg1->get(1);
    ASSERT_TRUE(entry == NULL);

    // create and open
    ASSERT_EQ(0, seg1->create());
    ASSERT_TRUE(seg1->is_open());

    // append entry
    for (int i = 0; i < 10; i++) {
        melon::raft::LogEntry* entry = new melon::raft::LogEntry();
        entry->AddRef();
        entry->type = melon::raft::ENTRY_TYPE_DATA;
        entry->id.term = 1;
        entry->id.index = i + 1;

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        entry->data.append(data_buf);

        ASSERT_EQ(0, seg1->append(entry));

        entry->Release();
    }

    // read entry
    for (int i = 0; i < 10; i++) {
        int64_t term = seg1->get_term(i+1);
        ASSERT_EQ(term, 1);

        melon::raft::LogEntry* entry = seg1->get(i+1);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, i+1);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }
    {
        melon::raft::LogEntry* entry = seg1->get(0);
        ASSERT_TRUE(entry == NULL);
        entry = seg1->get(11);
        ASSERT_TRUE(entry == NULL);
    }

    melon::raft::ConfigurationManager* configuration_manager = new melon::raft::ConfigurationManager;
    // load open segment
    melon::raft::Segment* seg2 = new melon::raft::Segment("./data", 1, 0);
    ASSERT_EQ(0, seg2->load(configuration_manager));

    for (int i = 0; i < 10; i++) {
        melon::raft::LogEntry* entry = seg2->get(i+1);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, i+1);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }
    {
        melon::raft::LogEntry* entry = seg2->get(0);
        ASSERT_TRUE(entry == NULL);
        entry = seg2->get(11);
        ASSERT_TRUE(entry == NULL);
    }
    delete seg2;

    // truncate and read
    ASSERT_EQ(0, seg1->truncate(5));
    for (int i = 0; i < 5; i++) {
        melon::raft::LogEntry* entry = new melon::raft::LogEntry();
        entry->type = melon::raft::ENTRY_TYPE_DATA;
        entry->id.term = 1;
        entry->id.index = i + 6;

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "HELLO, WORLD: %d", i + 6);
        entry->data.append(data_buf); 

        ASSERT_EQ(0, seg1->append(entry));

        entry->Release();
    }
    for (int i = 0; i < 10; i++) {
        melon::raft::LogEntry* entry = seg1->get(i+1);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, i+1);

        char data_buf[128];
        if (i < 5) {
            snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        } else {
            snprintf(data_buf, sizeof(data_buf), "HELLO, WORLD: %d", i + 1);
        }
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    ASSERT_EQ(0, seg1->close());
    ASSERT_FALSE(seg1->is_open());
    ASSERT_EQ(0, seg1->unlink());

    delete configuration_manager;
}

TEST_F(LogStorageTest, closed_segment) {
    // open segment operation
    melon::raft::Segment* seg1 = new melon::raft::Segment("./data", 1L, 0);
    ASSERT_EQ(0, seg1->create());
    ASSERT_TRUE(seg1->is_open());
    // append entry
    for (int i = 0; i < 10; i++) {
        melon::raft::LogEntry* entry = new melon::raft::LogEntry();
        entry->type = melon::raft::ENTRY_TYPE_DATA;
        entry->id.term = 1;
        entry->id.index = i + 1;

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        entry->data.append(data_buf);

        ASSERT_EQ(0, seg1->append(entry));

        entry->Release();
    }
    seg1->close();

    // read entry
    for (int i = 0; i < 10; i++) {
        melon::raft::LogEntry* entry = seg1->get(i+1);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, i+1);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }
    {
        melon::raft::LogEntry* entry = seg1->get(0);
        ASSERT_TRUE(entry == NULL);
        entry = seg1->get(11);
        ASSERT_TRUE(entry == NULL);
    }

    melon::raft::ConfigurationManager* configuration_manager = new melon::raft::ConfigurationManager;
    // load open segment
    melon::raft::Segment* seg2 = new melon::raft::Segment("./data", 1, 10, 0);
    ASSERT_EQ(0, seg2->load(configuration_manager));

    for (int i = 0; i < 10; i++) {
        melon::raft::LogEntry* entry = seg2->get(i+1);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, i+1);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }
    {
        melon::raft::LogEntry* entry = seg2->get(0);
        ASSERT_TRUE(entry == NULL);
        entry = seg2->get(11);
        ASSERT_TRUE(entry == NULL);
    }
    delete seg2;

    // truncate and read
    ASSERT_EQ(0, seg1->truncate(5));
    for (int i = 0; i < 5; i++) {
        melon::raft::LogEntry* entry = new melon::raft::LogEntry();
        entry->type = melon::raft::ENTRY_TYPE_DATA;
        entry->id.term = 1;
        entry->id.index = i + 6;

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "HELLO, WORLD: %d", i + 6);
        entry->data.append(data_buf);

        // become open segment again
        ASSERT_EQ(0, seg1->append(entry));

        entry->Release();
    }
    for (int i = 0; i < 10; i++) {
        melon::raft::LogEntry* entry = seg1->get(i+1);
        char data_buf[128];
        if (i < 5) {
            snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        } else {
            snprintf(data_buf, sizeof(data_buf), "HELLO, WORLD: %d", i + 1);
        }
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, i+1);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    ASSERT_EQ(0, seg1->unlink());

    delete configuration_manager;
}

TEST_F(LogStorageTest, multi_segment_and_segment_logstorage) {
    ::system("rm -rf data");
    melon::raft::SegmentLogStorage* storage = new melon::raft::SegmentLogStorage("./data");

    // init
    ASSERT_EQ(0, storage->init(new melon::raft::ConfigurationManager()));
    ASSERT_EQ(1, storage->first_log_index());
    ASSERT_EQ(0, storage->last_log_index());

    // append entry
    for (int i = 0; i < 100000; i++) {
        std::vector<melon::raft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            melon::raft::LogEntry* entry = new melon::raft::LogEntry();
            entry->type = melon::raft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %" PRId64, index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries, NULL));

        for (size_t j = 0; j < entries.size(); j++) {
            entries[j]->Release();
        }
    }

    // read entry
    for (int i = 0; i < 500000; i++) {
        int64_t index = i + 1;
        melon::raft::LogEntry* entry = storage->get_entry(index);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, index);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %" PRId64, index);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    ASSERT_EQ(storage->first_log_index(), 1);
    ASSERT_EQ(storage->last_log_index(), 500000);
    // truncate prefix
    ASSERT_EQ(0, storage->truncate_prefix(10001));
    ASSERT_EQ(storage->first_log_index(), 10001);
    ASSERT_EQ(storage->last_log_index(), 500000);

    // boundary truncate prefix
    {
        melon::raft::SegmentLogStorage::SegmentMap segments1 = storage->segments();
        size_t old_segment_num = segments1.size();
        melon::raft::Segment* first_seg = segments1.begin()->second.get();

        ASSERT_EQ(0, storage->truncate_prefix(first_seg->last_index()));
        melon::raft::SegmentLogStorage::SegmentMap segments2 = storage->segments();
        ASSERT_EQ(old_segment_num, segments2.size());

        ASSERT_EQ(0, storage->truncate_prefix(first_seg->last_index() + 1));
        melon::raft::SegmentLogStorage::SegmentMap segments3 = storage->segments();
        ASSERT_EQ(old_segment_num - 1, segments3.size());
    }

    ASSERT_EQ(0, storage->truncate_prefix(250001));
    ASSERT_EQ(storage->first_log_index(), 250001);
    ASSERT_EQ(storage->last_log_index(), 500000);
    for (int i = 250001; i <= 500000; i++) {
        int64_t index = i;
        melon::raft::LogEntry* entry = storage->get_entry(index);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, index);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %" PRId64, index);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    // append
    for (int i = 100000; i < 200000; i++) {
        std::vector<melon::raft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            melon::raft::LogEntry* entry = new melon::raft::LogEntry();
            entry->type = melon::raft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %" PRId64, index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries, NULL));

        for (size_t j = 0; j < entries.size(); j++) {
            entries[j]->Release();
        }
    }

    // truncate suffix
    ASSERT_EQ(250001, storage->first_log_index());
    ASSERT_EQ(1000000, storage->last_log_index());
    ASSERT_EQ(0, storage->truncate_suffix(750000));
    ASSERT_EQ(250001, storage->first_log_index());
    ASSERT_EQ(750000, storage->last_log_index());

    // boundary truncate suffix
    {
        melon::raft::SegmentLogStorage::SegmentMap segments1 = storage->segments();
        melon::raft::Segment* first_seg = segments1.begin()->second.get();
        if (segments1.size() > 1) {
            storage->truncate_suffix(first_seg->last_index() + 1);
        }
        melon::raft::SegmentLogStorage::SegmentMap segments2 = storage->segments();
        ASSERT_EQ(1ul, segments2.size());
        ASSERT_EQ(storage->last_log_index(), first_seg->last_index() + 1);
        storage->truncate_suffix(first_seg->last_index());
        melon::raft::SegmentLogStorage::SegmentMap segments3 = storage->segments();
        ASSERT_EQ(1ul, segments3.size());
        ASSERT_EQ(storage->last_log_index(), first_seg->last_index());
    }

    // read
    for (int i = 250001; i <= storage->last_log_index(); i++) {
        int64_t index = i;
        melon::raft::LogEntry* entry = storage->get_entry(index);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, index);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %" PRId64, index);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    delete storage;

    // re load
    ::system("rm -rf data/log_meta");
    melon::raft::SegmentLogStorage* storage2 = new melon::raft::SegmentLogStorage("./data");
    ASSERT_EQ(0, storage2->init(new melon::raft::ConfigurationManager()));
    ASSERT_EQ(1, storage2->first_log_index());
    ASSERT_EQ(0, storage2->last_log_index());
    delete storage2;
}

TEST_F(LogStorageTest, append_close_load_append) {
    ::system("rm -rf data");
    melon::raft::LogStorage* storage = new melon::raft::SegmentLogStorage("./data");
    melon::raft::ConfigurationManager* configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    // append entry
    for (int i = 0; i < 100000; i++) {
        std::vector<melon::raft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            melon::raft::LogEntry* entry = new melon::raft::LogEntry();
            entry->type = melon::raft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %" PRId64, index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries, NULL));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    delete storage;
    delete configuration_manager;

    // reinit 
    storage = new melon::raft::SegmentLogStorage("./data");
    configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    // append entry
    for (int i = 100000; i < 200000; i++) {
        std::vector<melon::raft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            melon::raft::LogEntry* entry = new melon::raft::LogEntry();
            entry->type = melon::raft::ENTRY_TYPE_DATA;
            entry->id.term = 2;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %" PRId64, index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries, NULL));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    // check and read
    ASSERT_EQ(storage->first_log_index(), 1);
    ASSERT_EQ(storage->last_log_index(), 200000*5);

    for (int i = 0; i < 200000*5; i++) {
        int64_t index = i + 1;
        melon::raft::LogEntry* entry = storage->get_entry(index);
        if (i < 100000*5) {
            ASSERT_EQ(entry->id.term, 1);
        } else {
            ASSERT_EQ(entry->id.term, 2);
        }
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, index);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %" PRId64, index);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    delete storage;
    delete configuration_manager;
}

ssize_t file_size(const char* filename) {
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

int truncate_uninterrupted(const char* filename, off_t length) {
    int rc = 0;
    do {
        rc = truncate(filename, length);
    } while (rc == -1 && errno == EINTR);
    return rc;
}

TEST_F(LogStorageTest, data_lost) {
    ::system("rm -rf data");
    melon::raft::LogStorage* storage = new melon::raft::SegmentLogStorage("./data");
    melon::raft::ConfigurationManager* configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    // append entry
    for (int i = 0; i < 100000; i++) {
        std::vector<melon::raft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            melon::raft::LogEntry* entry = new melon::raft::LogEntry();
            entry->type = melon::raft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries, NULL));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    delete storage;
    delete configuration_manager;

    // reinit 
    storage = new melon::raft::SegmentLogStorage("./data");
    configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    ASSERT_EQ(storage->first_log_index(), 1);
    ASSERT_EQ(storage->last_log_index(), 100000*5);

    delete storage;
    delete configuration_manager;

    // last segment lost data
    mutil::DirReaderPosix dir_reader1("./data");
    ASSERT_TRUE(dir_reader1.IsValid());
    while (dir_reader1.Next()) {
        int64_t first_index = 0;
        int match = sscanf(dir_reader1.name(), "log_inprogress_%020ld", 
                           &first_index);
        std::string path;
        mutil::string_appendf(&path, "./data/%s", dir_reader1.name());
        if (match == 1) {
            ASSERT_EQ(truncate_uninterrupted(path.c_str(), file_size(path.c_str()) - 1), 0);
        }
    }

    storage = new melon::raft::SegmentLogStorage("./data");
    configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    ASSERT_EQ(storage->first_log_index(), 1);
    ASSERT_EQ(storage->last_log_index(), 100000*5 - 1);

    delete storage;
    delete configuration_manager;

    // middle segment lost data
    mutil::DirReaderPosix dir_reader2("./data");
    ASSERT_TRUE(dir_reader2.IsValid());
    while (dir_reader2.Next()) {
        int64_t first_index = 0;
        int64_t last_index = 0;
        int match = sscanf(dir_reader2.name(), "log_%020ld_%020ld", 
                           &first_index, &last_index);
        std::string path;
        mutil::string_appendf(&path, "./data/%s", dir_reader2.name());
        if (match == 2) {
            ASSERT_EQ(truncate_uninterrupted(path.c_str(), file_size(path.c_str()) - 1), 0);
        }
    }

    storage = new melon::raft::SegmentLogStorage("./data");
    configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_NE(0, storage->init(configuration_manager));

    delete storage;
    delete configuration_manager;
}

TEST_F(LogStorageTest, full_segment_has_garbage) {
    ::system("rm -rf data");
    melon::raft::LogStorage* storage = new melon::raft::SegmentLogStorage("./data");
    melon::raft::ConfigurationManager* configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    // append entry
    for (int i = 0; i < 100000; i++) {
        std::vector<melon::raft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            melon::raft::LogEntry* entry = new melon::raft::LogEntry();
            entry->type = melon::raft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries, NULL));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    delete storage;
    delete configuration_manager;

    // generate garbage entries
    mutil::DirReaderPosix dir_reader("./data");
    std::string first_segment;
    std::string second_segment;
    while (dir_reader.IsValid()) {
        dir_reader.Next();
        int64_t first_index = 0;
        int64_t last_index = 0;
        int match = sscanf(dir_reader.name(), "log_%020ld_%020ld", 
                           &first_index, &last_index);
        if (match != 2) {
            continue;
        }
        if (first_segment.empty()) {
            mutil::string_appendf(&first_segment, "./data/%s", dir_reader.name());
        } else {
            mutil::string_appendf(&second_segment, "./data/%s", dir_reader.name());
            break;
        }
    }

    int fd1 = open(first_segment.c_str(), O_RDWR);
    int fd2 = open(second_segment.c_str(), O_RDWR);
    int off1 = 0;
    int off2 = 0;
    ASSERT_TRUE(fd1 >= 0);
    ASSERT_TRUE(fd2 >= 0);
    struct stat st_buf;
    ASSERT_EQ(fstat(fd1, &st_buf), 0);
    off1 = st_buf.st_size;
    for (;;) {
        mutil::IOPortal buf;
        ssize_t ret = melon::raft::file_pread(&buf, fd2, off2, 8192);
        ASSERT_TRUE(ret >= 0);
        if (ret == 0) {
            break;
        }
        ASSERT_EQ(buf.size(), melon::raft::file_pwrite(buf, fd1, off1));
        off1 += buf.size();
        off2 += buf.size();
    }
    close(fd1);
    close(fd2);

    storage = new melon::raft::SegmentLogStorage("./data");
    configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_NE(0, storage->init(configuration_manager));
}

TEST_F(LogStorageTest, append_read_badcase) {
    ::system("rm -rf data");
    melon::raft::LogStorage* storage = new melon::raft::SegmentLogStorage("./data");
    melon::raft::ConfigurationManager* configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    // append entry
    for (int i = 0; i < 100000; i++) {
        std::vector<melon::raft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            melon::raft::LogEntry* entry = new melon::raft::LogEntry();
            entry->type = melon::raft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %" PRId64, index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries, NULL));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    // check and read
    ASSERT_EQ(storage->first_log_index(), 1);
    ASSERT_EQ(storage->last_log_index(), 100000*5);

    delete storage;
    delete configuration_manager;

    // make file unwrite
    mutil::FileEnumerator dir1(mutil::FilePath("./data"), false,
                              mutil::FileEnumerator::FILES
                              | mutil::FileEnumerator::DIRECTORIES);
    for (mutil::FilePath sub_path = dir1.Next(); !sub_path.empty(); sub_path = dir1.Next()) {
        mutil::File::Info info;
        mutil::GetFileInfo(sub_path, &info);
        if (!info.is_directory) {
            chmod(sub_path.value().c_str(), 0444);
        }
    }

    // reinit failed, because load open no permission
    storage = new melon::raft::SegmentLogStorage("./data");
    configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_NE(0, storage->init(configuration_manager));
    delete storage;
    delete configuration_manager;

    mutil::FileEnumerator dir2(mutil::FilePath("./data"), false,
                              mutil::FileEnumerator::FILES
                              | mutil::FileEnumerator::DIRECTORIES);
    for (mutil::FilePath sub_path = dir2.Next(); !sub_path.empty(); sub_path = dir2.Next()) {
        mutil::File::Info info;
        mutil::GetFileInfo(sub_path, &info);
        if (!info.is_directory) {
            chmod(sub_path.value().c_str(), 0644);
        }
    }

    // reinit success
    storage = new melon::raft::SegmentLogStorage("./data");
    configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    // make file chaos
    mutil::FileEnumerator dir3(mutil::FilePath("./data"), false,
                              mutil::FileEnumerator::FILES
                              | mutil::FileEnumerator::DIRECTORIES);
    for (mutil::FilePath sub_path = dir3.Next(); !sub_path.empty(); sub_path = dir3.Next()) {
        mutil::File::Info info;
        mutil::GetFileInfo(sub_path, &info);
        if (!info.is_directory) {
            chmod(sub_path.value().c_str(), 0644);

            int fd = ::open(sub_path.value().c_str(), O_RDWR, 0644);
            int64_t off = rand() % info.size;
            int64_t len = rand() % (info.size - off);
            if (len > 4096) {
                len = 4096;
            }
            char data[4096] = {0};
            ::pwrite(fd, data, len, off);
            ::close(fd);
        }
    }

    // read will fail
    for (int i = 0; i < 100000*5; i++) {
        int64_t index = i + 1;
        melon::raft::LogEntry* entry = storage->get_entry(index);
        if (entry) {
            entry->Release();
        }
    }

    delete storage;
    delete configuration_manager;
}

TEST_F(LogStorageTest, configuration) {
    ::system("rm -rf data");
    melon::raft::SegmentLogStorage* storage = new melon::raft::SegmentLogStorage("./data");
    melon::raft::ConfigurationManager* configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    {
        melon::raft::LogEntry entry;
        entry.type = melon::raft::ENTRY_TYPE_NO_OP;
        entry.id.term = 1;
        entry.id.index = 1;

        ASSERT_EQ(0, storage->append_entry(&entry));
    }

    // add peer
    {
        melon::raft::LogEntry entry;
        entry.type = melon::raft::ENTRY_TYPE_CONFIGURATION;
        entry.id.term = 1;
        entry.id.index = 2;
        entry.peers = new std::vector<melon::raft::PeerId>;
        entry.peers->push_back(melon::raft::PeerId("1.1.1.1:1000:0"));
        entry.peers->push_back(melon::raft::PeerId("1.1.1.1:2000:0"));
        entry.peers->push_back(melon::raft::PeerId("1.1.1.1:3000:0"));
        storage->append_entry(&entry);
    }

    // append entry
    for (int i = 0; i < 100000; i++) {
        std::vector<melon::raft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 3 + i*5+j;
            melon::raft::LogEntry* entry = new melon::raft::LogEntry();
            entry->type = melon::raft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %" PRId64, index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }
        ASSERT_EQ(5, storage->append_entries(entries, NULL));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    // remove peer
    {
        int64_t index = 2 + 100000*5 + 1;
        melon::raft::LogEntry entry;
        entry.type = melon::raft::ENTRY_TYPE_CONFIGURATION;
        entry.id.term = 1;
        entry.id.index = index;
        entry.peers = new std::vector<melon::raft::PeerId>;
        entry.peers->push_back(melon::raft::PeerId("1.1.1.1:1000:0"));
        entry.peers->push_back(melon::raft::PeerId("1.1.1.1:2000:0"));
        storage->append_entry(&entry);
    }

    delete storage;

    melon::raft::SegmentLogStorage* storage2 = new melon::raft::SegmentLogStorage("./data");
    ASSERT_EQ(0, storage2->init(configuration_manager));

    melon::raft::ConfigurationEntry pair;
    configuration_manager->get(2 + 100000*5, &pair);
    ASSERT_EQ(2, pair.id.index);
    LOG(INFO) << pair.conf;

    configuration_manager->get(2 + 100000*5 + 1, &pair);
    ASSERT_EQ(2+100000*5+1, pair.id.index);
    LOG(INFO) << pair.conf;

    storage2->truncate_suffix(400000);
    configuration_manager->get(400000, &pair);
    ASSERT_EQ(2, pair.id.index);

    storage2->truncate_prefix(2);
    configuration_manager->get(400000, &pair);
    ASSERT_EQ(2, pair.id.index);

    delete storage2;
}

mutil::atomic<int> g_first_read_index(0);
mutil::atomic<int> g_last_read_index(0);
bool g_stop = false;

void* read_thread_routine(void* arg) {
    melon::raft::SegmentLogStorage* storage = (melon::raft::SegmentLogStorage*)arg;
    while (!g_stop) {
        int a = g_first_read_index.load(mutil::memory_order_relaxed);
        int b = g_last_read_index.load(mutil::memory_order_relaxed);
        EXPECT_LE(a, b);
        int index = mutil::fast_rand_in(a, b);
        melon::raft::LogEntry* entry = storage->get_entry(index);
        if (entry != NULL) {
            std::string expect;
            mutil::string_printf(&expect, "hello_%d", index);
            EXPECT_EQ(expect, entry->data.to_string());
            entry->Release();
        } else {
            EXPECT_LT(index, storage->first_log_index()) 
                    << "first_read_index=" << g_first_read_index.load()
                    << " last_read_index=" << g_last_read_index.load()
                    << " a=" << a << " b=" << b;
            g_stop = true;
            return NULL;
        }
    }
    return NULL;
}

void* write_thread_routine(void* arg) {
    melon::raft::SegmentLogStorage* storage = (melon::raft::SegmentLogStorage*)arg;
    // Write operation distribution: 
    //  - 10% truncate_prefix
    //  - 10% truncate_suffix,
    //  - 30% increase last_read_index (which stands for commitment in the real
    // world), 
    //  - 50% append new entry
    int next_log_index = storage->last_log_index() + 1;
    while (!g_stop) {
        const int r = mutil::fast_rand_in(0, 9);
        if (r < 1) {  // truncate_prefix
            int truncate_index = mutil::fast_rand_in(
                    g_first_read_index.load(mutil::memory_order_relaxed),
                    g_last_read_index.load(mutil::memory_order_relaxed));
            EXPECT_EQ(0, storage->truncate_prefix(truncate_index));
            g_first_read_index.store(truncate_index, mutil::memory_order_relaxed);
        } else if (r < 2) {  // truncate suffix
            int truncate_index = mutil::fast_rand_in(
                    g_last_read_index.load(mutil::memory_order_relaxed),
                    next_log_index - 1);
            EXPECT_EQ(0, storage->truncate_suffix(truncate_index));
            next_log_index = truncate_index + 1;
        } else if (r < 5) { // increase last_read_index which cannot be truncate
            int next_read_index = mutil::fast_rand_in(
                    g_last_read_index.load(mutil::memory_order_relaxed),
                    next_log_index - 1);
            g_last_read_index.store(next_read_index, mutil::memory_order_relaxed);
        } else  {  // Append entry
            melon::raft::LogEntry* entry = new melon::raft::LogEntry;
            entry->type = melon::raft::ENTRY_TYPE_DATA;
            entry->id.index = next_log_index;
            std::string data;
            mutil::string_printf(&data, "hello_%d", next_log_index);
            entry->data.append(data);
            ++next_log_index;
            EXPECT_EQ(0, storage->append_entry(entry));
            entry->Release();
        }
    }
    return NULL;
}

namespace melon::raft {
DECLARE_int32(raft_max_segment_size);
}

TEST_F(LogStorageTest, multi_read_single_modify_thread_safe) {
    int32_t saved_max_segment_size = melon::raft::FLAGS_raft_max_segment_size;
    melon::raft::FLAGS_raft_max_segment_size = 1024;
    system("rm -rf ./data");
    melon::raft::SegmentLogStorage* storage = new melon::raft::SegmentLogStorage("./data");
    melon::raft::ConfigurationManager* configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));
    const int N = 10000;
    for (int i = 1; i <= N; ++i) {
        melon::raft::LogEntry* entry = new melon::raft::LogEntry;
        entry->type = melon::raft::ENTRY_TYPE_DATA;
        entry->id.index = i;
        std::string data;
        mutil::string_printf(&data, "hello_%d", i);
        entry->data.append(data);
        ASSERT_EQ(0, storage->append_entry(entry));
        entry->Release();
    }
    ASSERT_EQ(N, storage->last_log_index());
    g_stop = false;
    g_first_read_index.store(1);
    g_last_read_index.store(N);
    fiber_t read_thread[8];
    for (size_t i = 0; i < ARRAY_SIZE(read_thread); ++i) {
        ASSERT_EQ(0, fiber_start_urgent(&read_thread[i], NULL,
                                   read_thread_routine, storage));
    }
    fiber_t write_thread;
    ASSERT_EQ(0, fiber_start_urgent(&write_thread, NULL,
                                      write_thread_routine, storage));
    ::usleep(5 * 1000 * 1000);
    g_stop = true;
    for (size_t i = 0; i < ARRAY_SIZE(read_thread); ++i) {
        fiber_join(read_thread[i], NULL);
    }
    fiber_join(write_thread, NULL);

    delete configuration_manager;
    delete storage;
    melon::raft::FLAGS_raft_max_segment_size = saved_max_segment_size;
}

TEST_F(LogStorageTest, max_segment_size_illegal) {
    int32_t saved_max_segment_size = melon::raft::FLAGS_raft_max_segment_size;
    melon::raft::FLAGS_raft_max_segment_size = -1;
    system("rm -rf ./data");
    melon::raft::SegmentLogStorage* storage = new melon::raft::SegmentLogStorage("./data");
    melon::raft::ConfigurationManager* configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(-1, storage->init(configuration_manager));
    delete configuration_manager;
    delete storage;
    melon::raft::FLAGS_raft_max_segment_size = saved_max_segment_size;
}

TEST_F(LogStorageTest, large_entry) {
    system("rm -rf ./data");
    melon::raft::SegmentLogStorage* storage = new melon::raft::SegmentLogStorage("./data");
    melon::raft::ConfigurationManager* configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));
    melon::raft::LogEntry* entry = new melon::raft::LogEntry;
    entry->type = melon::raft::ENTRY_TYPE_DATA;
    entry->id.index = 1;
    entry->id.term = 1;
    std::string data;
    data.resize(512 * 1024 * 1024, 'a');
    entry->data.append(data);
    ASSERT_EQ(0, storage->append_entry(entry));
    entry->Release();
    entry = storage->get_entry(1);
    ASSERT_EQ(data, entry->data.to_string());
    entry->Release();

    ASSERT_EQ(1, storage->_first_log_index); 
    ASSERT_EQ(1, storage->_last_log_index);
    ASSERT_EQ(0, storage->_segments.size());
    scoped_refptr<melon::raft::Segment> segment = storage->open_segment();
    ASSERT_EQ(1, storage->_segments.size());

    melon::raft::SegmentLogStorage* storage2 = new melon::raft::SegmentLogStorage("./data");
    melon::raft::ConfigurationManager* configuration_manager2 = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage2->init(configuration_manager2));
    ASSERT_EQ(1, storage2->_first_log_index); 
    ASSERT_EQ(1, storage2->_last_log_index);
    ASSERT_EQ(1, storage2->_segments.size());
}

TEST_F(LogStorageTest, reboot_with_checksum_type_changed) {
    system("rm -rf ./data");
    melon::raft::SegmentLogStorage* storage = new melon::raft::SegmentLogStorage("./data");
    melon::raft::ConfigurationManager* configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));
    storage->_checksum_type = 0;  // murmurhash
    for (int i = 0; i < 10; i++) {
        melon::raft::LogEntry* entry = new melon::raft::LogEntry();
        entry->type = melon::raft::ENTRY_TYPE_DATA;
        entry->id.term = 1;
        entry->id.index = i + 1;

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        entry->data.append(data_buf);

        ASSERT_EQ(0, storage->append_entry(entry));

        entry->Release();
    }
    delete storage;
    storage = new melon::raft::SegmentLogStorage("./data");
    ASSERT_EQ(0, storage->init(configuration_manager));
    storage->_checksum_type = 1;  // crc32
    for (int i = 10; i < 20; i++) {
        melon::raft::LogEntry* entry = new melon::raft::LogEntry();
        entry->type = melon::raft::ENTRY_TYPE_DATA;
        entry->id.term = 1;
        entry->id.index = i + 1;

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", i + 1);
        entry->data.append(data_buf);

        ASSERT_EQ(0, storage->append_entry(entry));

        entry->Release();
    }
    delete storage;
    storage = new melon::raft::SegmentLogStorage("./data");
    ASSERT_EQ(0, storage->init(configuration_manager));
    for (int index = 1; index <= 20; ++index) {
        melon::raft::LogEntry* entry = storage->get_entry(index);
        ASSERT_EQ(entry->id.term, 1);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, index);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %d", index);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }
    
    delete storage;
}

TEST_F(LogStorageTest, joint_configuration) {
    system("rm -rf ./data");
    melon::raft::ConfigurationManager cm;
    std::unique_ptr<melon::raft::SegmentLogStorage>
                log_storage(new melon::raft::SegmentLogStorage("./data"));
    ASSERT_EQ(0, log_storage->init(&cm));
    for (int i = 1; i <= 20; ++i) {
        scoped_refptr<melon::raft::LogEntry> entry = new melon::raft::LogEntry;
        entry->id = melon::raft::LogId(i, 1);
        entry->peers = new std::vector<melon::raft::PeerId>;
        entry->type = melon::raft::ENTRY_TYPE_CONFIGURATION;
        for (int j = 0; j < 3; ++j) {
            entry->peers->push_back("127.0.0.1:" + std::to_string(i + j));
        }
        entry->old_peers = new std::vector<melon::raft::PeerId>;
        for (int j = 1; j <= 3; ++j) {
            entry->old_peers->push_back("127.0.0.1:" + std::to_string(i + j));
        }
        ASSERT_EQ(0, log_storage->append_entry(entry));
    }

    for (int i = 1; i <= 20; ++i) {
        melon::raft::LogEntry* entry = log_storage->get_entry(i);
        ASSERT_TRUE(entry != NULL);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_CONFIGURATION);
        ASSERT_TRUE(entry->peers != NULL);
        ASSERT_TRUE(entry->old_peers != NULL);
        melon::raft::Configuration conf;
        for (int j = 0; j < 3; ++j) {
            conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        melon::raft::Configuration old_conf;
        for (int j = 1; j <= 3; ++j) {
            old_conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        ASSERT_TRUE(conf.equals(*entry->peers))
            << conf << " xxxx " << melon::raft::Configuration(*entry->peers);
                    
        ASSERT_TRUE(old_conf.equals(*entry->old_peers));
        entry->Release();
    }

    // Restart
    log_storage.reset(new melon::raft::SegmentLogStorage("./data"));
    ASSERT_EQ(0, log_storage->init(&cm));
    for (int i = 1; i <= 20; ++i) {
        melon::raft::LogEntry* entry = log_storage->get_entry(i);
        ASSERT_TRUE(entry != NULL);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_CONFIGURATION);
        ASSERT_TRUE(entry->peers != NULL);
        ASSERT_TRUE(entry->old_peers != NULL);
        melon::raft::Configuration conf;
        for (int j = 0; j < 3; ++j) {
            conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        melon::raft::Configuration old_conf;
        for (int j = 1; j <= 3; ++j) {
            old_conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        ASSERT_TRUE(conf.equals(*entry->peers))
            << conf << " xxxx " << melon::raft::Configuration(*entry->peers);
                    
        ASSERT_TRUE(old_conf.equals(*entry->old_peers));
        entry->Release();
    }

    for (int i = 1; i <= 20; ++i) {
        melon::raft::LogEntry* entry = log_storage->get_entry(i);
        ASSERT_TRUE(entry != NULL);
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_CONFIGURATION);
        ASSERT_TRUE(entry->peers != NULL);
        ASSERT_TRUE(entry->old_peers != NULL);
        ASSERT_EQ(1, entry->id.term);
        melon::raft::Configuration conf;
        for (int j = 0; j < 3; ++j) {
            conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        melon::raft::Configuration old_conf;
        for (int j = 1; j <= 3; ++j) {
            old_conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        ASSERT_TRUE(conf.equals(*entry->peers));
        ASSERT_TRUE(old_conf.equals(*entry->old_peers));
        entry->Release();
    }

    for (int i = 1; i <= 20; ++i) {
        melon::raft::ConfigurationEntry entry;
        cm.get(i, &entry);
        ASSERT_EQ(melon::raft::LogId(i, 1), entry.id);
        melon::raft::Configuration conf;
        for (int j = 0; j < 3; ++j) {
            conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        melon::raft::Configuration old_conf;
        for (int j = 1; j <= 3; ++j) {
            old_conf.add_peer("127.0.0.1:" + std::to_string(i + j));
        }
        ASSERT_TRUE(conf.equals(entry.conf));
        ASSERT_TRUE(old_conf.equals(entry.old_conf));
    }
}

TEST_F(LogStorageTest, append_close_load_append_with_io_metric) {
    ::system("rm -rf data");
    melon::raft::IOMetric metric;
    melon::raft::FLAGS_raft_trace_append_entry_latency = true;
    melon::raft::LogStorage* storage = new melon::raft::SegmentLogStorage("./data");
    melon::raft::ConfigurationManager* configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    // append entry
    for (int i = 0; i < 100000; i++) {
        std::vector<melon::raft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            melon::raft::LogEntry* entry = new melon::raft::LogEntry();
            entry->type = melon::raft::ENTRY_TYPE_DATA;
            entry->id.term = 1;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries, &metric));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    ASSERT_NE(0, metric.open_segment_time_us);
    ASSERT_NE(0, metric.append_entry_time_us);
    ASSERT_NE(0, metric.sync_segment_time_us);

    LOG(INFO) << metric;

    delete storage;
    delete configuration_manager;

    // reinit 
    storage = new melon::raft::SegmentLogStorage("./data");
    configuration_manager = new melon::raft::ConfigurationManager;
    ASSERT_EQ(0, storage->init(configuration_manager));

    // append entry
    for (int i = 100000; i < 200000; i++) {
        std::vector<melon::raft::LogEntry*> entries;
        for (int j = 0; j < 5; j++) {
            int64_t index = 5*i + j + 1;
            melon::raft::LogEntry* entry = new melon::raft::LogEntry();
            entry->type = melon::raft::ENTRY_TYPE_DATA;
            entry->id.term = 2;
            entry->id.index = index;

            char data_buf[128];
            snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
            entry->data.append(data_buf);
            entries.push_back(entry);
        }

        ASSERT_EQ(5, storage->append_entries(entries, &metric));

        for (size_t j = 0; j < entries.size(); j++) {
            delete entries[j];
        }
    }

    // check and read
    ASSERT_EQ(storage->first_log_index(), 1);
    ASSERT_EQ(storage->last_log_index(), 200000*5);

    for (int i = 0; i < 200000*5; i++) {
        int64_t index = i + 1;
        melon::raft::LogEntry* entry = storage->get_entry(index);
        if (i < 100000*5) {
            ASSERT_EQ(entry->id.term, 1);
        } else {
            ASSERT_EQ(entry->id.term, 2);
        }
        ASSERT_EQ(entry->type, melon::raft::ENTRY_TYPE_DATA);
        ASSERT_EQ(entry->id.index, index);

        char data_buf[128];
        snprintf(data_buf, sizeof(data_buf), "hello, world: %ld", index);
        ASSERT_EQ(data_buf, entry->data.to_string());
        entry->Release();
    }

    delete storage;
    delete configuration_manager;
}
