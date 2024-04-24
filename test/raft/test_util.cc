// libraft - Quorum-based replication of states across machines.
// Copyright (c) 2015 Baidu.com, Inc. All Rights Reserved


#include <gtest/gtest.h>
#include <melon/utility/logging.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <melon/utility/files/scoped_temp_dir.h>

#include "melon/raft/util.h"

class TestUsageSuits : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

using melon::raft::raft_mutex_t;

struct LockMeta {
    raft_mutex_t* mutex;
    int64_t value;
};

void* run_lock_guard(void *arg) {
    LockMeta* meta = (LockMeta*)arg;

    for (int i = 0; i < 10000; i++) {
        std::lock_guard<raft_mutex_t> guard(*(meta->mutex));
        meta->value++;
    }
    return NULL;
}

TEST_F(TestUsageSuits, lock) {
    raft_mutex_t mutex;

    // fiber lock guard
    LockMeta meta;
    meta.value = 0;
    meta.mutex = &mutex;

    fiber_t tids[10];
    for (int i = 0; i < 10; i++) {
        fiber_start_background(&tids[i], &FIBER_ATTR_NORMAL, run_lock_guard, &meta);
    }

    for (int i = 0; i < 10; i++) {
        fiber_join(tids[i], NULL);
    }

    ASSERT_EQ(meta.value, 10*10000);
}

TEST_F(TestUsageSuits, murmurhash) {
    char* data = (char*)malloc(1024*1024);
    for (int i = 0; i < 1024*1024; i++) {
        data[i] = 'a' + i % 26;
    }
    int32_t val1 = melon::raft::murmurhash32(data, 1024*1024);

    mutil::IOBuf buf;
    for (int i = 0; i < 1024 * 1024; i++) {
        char c = 'a' + i % 26;
        buf.push_back(c);
    }
    int32_t val2 = melon::raft::murmurhash32(buf);
    ASSERT_EQ(val1, val2);
    free(data);
}

TEST_F(TestUsageSuits, pread_pwrite) {
    int fd = ::open("./pread_pwrite.data", O_CREAT | O_TRUNC | O_RDWR, 0644);

    mutil::IOPortal portal;
    ssize_t nread = melon::raft::file_pread(&portal, fd, 1000, 10);
    ASSERT_EQ(nread, 0);

    mutil::IOBuf data;
    data.append("hello");
    ssize_t nwritten = melon::raft::file_pwrite(data, fd, 1000);
    ASSERT_EQ(nwritten, data.size());

    portal.clear();
    nread = melon::raft::file_pread(&portal, fd, 1000, 10);
    ASSERT_EQ(nread, data.size());
    ASSERT_EQ(melon::raft::murmurhash32(data), melon::raft::murmurhash32(portal));

    ::close(fd);
    ::unlink("./pread_pwrite.data");
}

TEST_F(TestUsageSuits, FileSegData) {
    melon::raft::FileSegData seg_writer;
    for (uint64_t i = 0; i < 10UL; i++) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "raw hello %" PRIu64, i);
        seg_writer.append(buf, 1000 * i, strlen(buf));
    }
    for (uint64_t i = 10; i < 20UL; i++) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "iobuf hello %" PRIu64, i);
        mutil::IOBuf piece_buf;
        piece_buf.append(buf, strlen(buf));
        seg_writer.append(piece_buf, 1000 * i);
    }

    melon::raft::FileSegData seg_reader(seg_writer.data());
    uint64_t seg_offset = 0;
    mutil::IOBuf seg_data;
    uint64_t index = 0;
    while (0 != seg_reader.next(&seg_offset, &seg_data)) {
        ASSERT_EQ(index * 1000, seg_offset);

        char buf[1024] = {0};
        if (index < 10) {
            snprintf(buf, sizeof(buf), "raw hello %" PRIu64, index);
        } else {
            snprintf(buf, sizeof(buf), "iobuf hello %" PRIu64, index);
        }

        char new_buf[1024] = {0};
        seg_data.copy_to(new_buf, strlen(buf));
        printf("index:%" PRIu64 " old: %s new: %s\n", index, buf, new_buf);
        ASSERT_EQ(melon::raft::murmurhash32(seg_data), melon::raft::murmurhash32(buf, strlen(buf)));

        seg_data.clear();
        index ++;
    }
    ASSERT_EQ(index, 20UL);
}

TEST_F(TestUsageSuits, crc32) {
    char* data = (char*)malloc(1024*1024);
    for (int i = 0; i < 1024*1024; i++) {
        data[i] = 'a' + i % 26;
    }
    int32_t val1 = melon::raft::crc32(data, 1024*1024);

    mutil::IOBuf buf;
    for (int i = 0; i < 1024 * 1024; i++) {
        char c = 'a' + i % 26;
        buf.push_back(c);
    }
    int32_t val2 = melon::raft::crc32(buf);
    ASSERT_EQ(val1, val2);

    free(data);
}

bool is_zero1(const char *buff, size_t size) {
    while (size--) {
        if (*buff++) {
            return false;
        }
    }
    return true;
}

int is_zero2(const char *buff, const size_t size) {
    return (0 == *buff && 0 == memcmp(buff, buff + 1, size - 1));
}

int is_zero3(const char *buff, const size_t size) {
    return (0 == *(uint64_t*)buff &&
            0 == memcmp(buff, buff + sizeof(uint64_t), size - sizeof(uint64_t)));
}

int is_zero4(const char *buff, const size_t size) {
    return (0 == *(wchar_t*)buff &&
            0 ==  wmemcmp((wchar_t*)buff, (wchar_t*)buff + 1, size / sizeof(wchar_t) - 1));
}

int is_zero5(const char *buff, size_t size) {
    for (size_t i = 0; i < size / sizeof(uint64_t); i++) {
        if (*(uint64_t*)buff != 0) {
            return false;
        }
        buff += sizeof(uint64_t);
    }
    size %= sizeof(uint64_t);
    for (size_t i = 0; i < size / sizeof(uint8_t); i++) {
        if (*(uint8_t*)buff != 0) {
            return false;
        }
        buff += sizeof(uint8_t);
    }
    return true;
}

int is_zero_memcmp(const char* buff, size_t size) {
    static char static_zero_1m_buf[1024*1024] = {0};
    return 0 == memcmp(buff, static_zero_1m_buf, size);
}

#define IS_ZERO_TEST(func, size)                                               \
    do {                                                                       \
        int64_t start = mutil::detail::clock_cycles();                          \
        ASSERT_TRUE(func(data, size));                                         \
        int64_t end = mutil::detail::clock_cycles();                            \
        MLOG(INFO) << #func << " cycle: " << end - start;     \
    } while (0)

TEST_F(TestUsageSuits, is_zero) {
    char* data = (char*)malloc(1024*1024);
    memset(data, 0, 1024*1024);

    {
        char* tmp_data = (char*)malloc(1024*1024);
        memset(tmp_data, 'a', 1024*1024);
        free(tmp_data);
        ASSERT_EQ(0, memcmp(data, tmp_data, 0));
    }

    int test_sizes[] = {4*1024, 8*1024, 16*1024, 64*1024, 128*1024, 256*1024, 512*1024, 1024*1024};
    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(int); i++) {
        MLOG(INFO) << "is_zero size: " << test_sizes[i];
        IS_ZERO_TEST(is_zero1, test_sizes[i]);
        IS_ZERO_TEST(is_zero2, test_sizes[i]);
        IS_ZERO_TEST(is_zero3, test_sizes[i]);
        IS_ZERO_TEST(is_zero4, test_sizes[i]);
        IS_ZERO_TEST(is_zero5, test_sizes[i]);
        IS_ZERO_TEST(is_zero_memcmp, test_sizes[i]);
        IS_ZERO_TEST(melon::raft::is_zero, test_sizes[i]);
    }

    for (int i = 1024; i >= 1; i--) {
        ASSERT_TRUE(melon::raft::is_zero(data, i*1024));
    }
    for (int i = 1; i < 8; i++) {
        ASSERT_TRUE(melon::raft::is_zero(data, i));
    }

    int rand_pos = rand() % (1024 * 1024);
    data[rand_pos] = 'a' + rand() % 26;
    ASSERT_FALSE(melon::raft::is_zero(data, 1024 * 1024));
    ASSERT_TRUE(melon::raft::is_zero(data, rand_pos));
    ASSERT_TRUE(melon::raft::is_zero(data + rand_pos + 1, 1024 * 1024 - 1 - rand_pos));

    memset(data, 0, 1024*1024);
    rand_pos = rand() % 8;
    data[rand_pos] = 'a' + rand() % 26;
    ASSERT_FALSE(melon::raft::is_zero(data, 8));
    ASSERT_TRUE(melon::raft::is_zero(data, rand_pos));
    ASSERT_TRUE(melon::raft::is_zero(data + rand_pos + 1, 8 - 1 - rand_pos));

    free(data);
}

TEST_F(TestUsageSuits, file_path) {
    mutil::FilePath path("dir/");
    MLOG(INFO) << "dir_name=" << path.DirName().value()
              << " base_name=" << path.BaseName().value();
    path = mutil::FilePath("dir");
    MLOG(INFO) << "dir_name=" << path.DirName().value()
              << " base_name=" << path.BaseName().value();
    path = mutil::FilePath("../sub4/sub5/dir");
    MLOG(INFO) << path.ReferencesParent();
}

