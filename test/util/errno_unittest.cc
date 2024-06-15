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


#include <gtest/gtest.h>
#include <melon/base/errno.h>

class ErrnoTest : public ::testing::Test{
protected:
    ErrnoTest(){
    };
    virtual ~ErrnoTest(){};
    virtual void SetUp() {
    };
    virtual void TearDown() {
    };
};

#define ESTOP -114
#define EBREAK -115
#define ESTH -116
#define EOK -117
#define EMYERROR -30

MELON_REGISTER_ERRNO(ESTOP, "the thread is stopping")
MELON_REGISTER_ERRNO(EBREAK, "the thread is interrupted")
MELON_REGISTER_ERRNO(ESTH, "something happened")
MELON_REGISTER_ERRNO(EOK, "OK!")
MELON_REGISTER_ERRNO(EMYERROR, "my error");

TEST_F(ErrnoTest, system_errno) {
    errno = EPIPE;
    ASSERT_STREQ("Broken pipe", berror());
    ASSERT_STREQ("Interrupted system call", berror(EINTR));
}

TEST_F(ErrnoTest, customized_errno) {
    ASSERT_STREQ("the thread is stopping", berror(ESTOP));
    ASSERT_STREQ("the thread is interrupted", berror(EBREAK));
    ASSERT_STREQ("something happened", berror(ESTH));
    ASSERT_STREQ("OK!", berror(EOK));
    ASSERT_STREQ("Unknown error 1000", berror(1000));
    
    errno = ESTOP;
    printf("Something got wrong, %m\n");
    printf("Something got wrong, %s\n", berror());
}
