//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#include <sys/types.h>                          // open
#include <sys/stat.h>                           // ^
#include <fcntl.h>                              // ^
#include <gtest/gtest.h>
#include <errno.h>
#include <melon/base/fd_guard.h>

namespace {

class FDGuardTest : public ::testing::Test{
protected:
    FDGuardTest(){
    };
    virtual ~FDGuardTest(){};
    virtual void SetUp() {
    };
    virtual void TearDown() {
    };
};

TEST_F(FDGuardTest, default_constructor) {
    mutil::fd_guard guard;
    ASSERT_EQ(-1, guard);
}

TEST_F(FDGuardTest, destructor_closes_fd) {
    int fd = -1;
    {
        mutil::fd_guard guard(open(".tmp1",  O_WRONLY|O_CREAT, 0600));
        ASSERT_GT(guard, 0);
        fd = guard;
    }
    char dummy = 0;
    ASSERT_EQ(-1L, write(fd, &dummy, 1));
    ASSERT_EQ(EBADF, errno);
}

TEST_F(FDGuardTest, reset_closes_previous_fd) {
    mutil::fd_guard guard(open(".tmp1",  O_WRONLY|O_CREAT, 0600));
    ASSERT_GT(guard, 0);
    const int fd = guard;
    const int fd2 = open(".tmp2",  O_WRONLY|O_CREAT, 0600);
    guard.reset(fd2);
    char dummy = 0;
    ASSERT_EQ(-1L, write(fd, &dummy, 1));
    ASSERT_EQ(EBADF, errno);
    guard.reset(-1);
    ASSERT_EQ(-1L, write(fd2, &dummy, 1));
    ASSERT_EQ(EBADF, errno);
}
    
TEST_F(FDGuardTest, release) {
    mutil::fd_guard guard(open(".tmp1",  O_WRONLY|O_CREAT, 0600));
    ASSERT_GT(guard, 0);
    const int fd = guard;
    ASSERT_EQ(fd, guard.release());
    ASSERT_EQ(-1, guard);
    close(fd);
}
}
