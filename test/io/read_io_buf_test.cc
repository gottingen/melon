//
// Created by liyinbin on 2021/5/1.
//


#include "abel/io/read_iobuf.h"

#include <fcntl.h>
#include <unistd.h>

#include <memory>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "abel/base/random.h"
#include "abel/io/fd_utility.h"

using namespace std::literals;

namespace abel {

    class read_iobuf_test : public ::testing::Test {
    public:
        void SetUp() override {
            DCHECK(pipe(fd_) == 0);
            DCHECK(write(fd_[1], "1234567", 7) == 7);
            make_non_blocking(fd_[0]);
            make_non_blocking(fd_[1]);
            io_ = std::make_unique<system_io_stream>(fd_[0]);
            buffer_.clear();
        }

        void TearDown() override {
            // Failure is ignored. If the testcase itself closed the socket, call(s)
            // below can fail.
            close(fd_[0]);
            close(fd_[1]);
            DLOG_INFO("leaving");
        }

    protected:
        int fd_[2];  // read fd, write fd.
        std::unique_ptr<system_io_stream> io_;
        iobuf buffer_;
        std::size_t bytes_read_;
    };

    TEST_F(read_iobuf_test, Drained) {
        ASSERT_EQ(read_status::eDrained,
                  read_iobuf(8, io_.get(), &buffer_, &bytes_read_));
        EXPECT_EQ("1234567", flatten_slow(buffer_));
        EXPECT_EQ(7, bytes_read_);
    }

    TEST_F(read_iobuf_test, Drained2) {
        buffer_ = create_buffer_slow("0000");
        ASSERT_EQ(read_status::eDrained,
                  read_iobuf(8, io_.get(), &buffer_, &bytes_read_));
        EXPECT_EQ("00001234567", flatten_slow(buffer_));
        EXPECT_EQ(7, bytes_read_);
    }

    TEST_F(read_iobuf_test, MaxBytesRead) {
        ASSERT_EQ(read_status::eMaxBytesRead,
                  read_iobuf(7, io_.get(), &buffer_, &bytes_read_));
        EXPECT_EQ("1234567", flatten_slow(buffer_));
        EXPECT_EQ(7, bytes_read_);
    }

    TEST_F(read_iobuf_test, MaxBytesRead2) {
        ASSERT_EQ(read_status::eMaxBytesRead,
                  read_iobuf(5, io_.get(), &buffer_, &bytes_read_));
        EXPECT_EQ("12345", flatten_slow(buffer_));
        EXPECT_EQ(5, bytes_read_);
    }

    TEST_F(read_iobuf_test, PeerClosing) {
        DCHECK(close(fd_[1]) == 0);
        ASSERT_EQ(read_status::eDrained,
                  read_iobuf(8, io_.get(), &buffer_, &bytes_read_));
        EXPECT_EQ(7, bytes_read_);
        // This is weird. The first call always succeeds even if it can tell the
        // remote side has closed the socket, yet we still need to issue another call
        // to `read` to see the situation.
        ASSERT_EQ(read_status::eEof,
                  read_iobuf(1, io_.get(), &buffer_, &bytes_read_));
        EXPECT_EQ(0, bytes_read_);
        EXPECT_EQ("1234567", flatten_slow(buffer_));
    }

/*
    TEST(read_iobuf, LargeChunk) {


        constexpr auto kMaxBytes = 76301;

        int fd[2];  // read fd, write fd.
        DCHECK(pipe(fd) == 0);
#if defined(ABEL_PLATFORM_LINUX)
        DCHECK(fcntl(fd[0], F_SETPIPE_SZ, kMaxBytes) == kMaxBytes);
#endif
        make_non_blocking(fd[0]);
        make_non_blocking(fd[1]);
        auto io = std::make_unique<system_io_stream>(fd[0]);

        std::string source;
        for (int i = 0; i != kMaxBytes; ++i) {
            source.push_back(Random<char>());
        }

        for (int i = 1; i < kMaxBytes; i = i * 3 / 2 + 17) {
            DLOG_INFO("Testing chunk of size {}.", i);

            DCHECK_LE(write(fd[1], source.data(), i), i);

            iobuf buffer;
            std::size_t bytes_read;

            if (Random() % 2 == 0) {
                ASSERT_EQ(read_status::eDrained,
                          read_iobuf(i + 1, io.get(), &buffer, &bytes_read));
            } else {
                ASSERT_EQ(read_status::eMaxBytesRead,
                          read_iobuf(i, io.get(), &buffer, &bytes_read));
            }
            EXPECT_EQ(i, bytes_read);
            // Not using `EXPECT_EQ` as diagnostics on error is potentially large, so we
            // want to bail out on error ASAP.
            ASSERT_EQ(flatten_slow(buffer), source.substr(0, i));
        }
    }
    */

}  // namespace abel