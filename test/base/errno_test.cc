
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/gtest_wrap.h"
#include "melon/base/errno.h"

class ErrnoTest : public ::testing::Test {
protected:
    ErrnoTest() {
    };

    virtual ~ErrnoTest() {};

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
    ASSERT_STREQ("Broken pipe", melon_error());
    ASSERT_STREQ("Interrupted system call", melon_error(EINTR));
}

TEST_F(ErrnoTest, customized_errno) {
    ASSERT_STREQ("the thread is stopping", melon_error(ESTOP));
    ASSERT_STREQ("the thread is interrupted", melon_error(EBREAK));
    ASSERT_STREQ("something happened", melon_error(ESTH));
    ASSERT_STREQ("OK!", melon_error(EOK));
    ASSERT_STREQ("Unknown error 1000", melon_error(1000));

    errno = ESTOP;
    printf("Something got wrong, %m\n");
    printf("Something got wrong, %s\n", melon_error());
}
