
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include <testing/filesystem_test_util.h>
#include "testing/gtest_wrap.h"

TEST(perm, all) {
    EXPECT_TRUE((melon::perms::owner_read | melon::perms::owner_write | melon::perms::owner_exec) == melon::perms::owner_all);
    EXPECT_TRUE((melon::perms::group_read | melon::perms::group_write | melon::perms::group_exec) == melon::perms::group_all);
    EXPECT_TRUE((melon::perms::others_read | melon::perms::others_write | melon::perms::others_exec) == melon::perms::others_all);
    EXPECT_TRUE((melon::perms::owner_all | melon::perms::group_all | melon::perms::others_all) == melon::perms::all);
    EXPECT_TRUE((melon::perms::all | melon::perms::set_uid | melon::perms::set_gid | melon::perms::sticky_bit) == melon::perms::mask);
}

TEST(file_status, all) {
    {
        melon::file_status fs;
        EXPECT_TRUE(fs.type() == melon::file_type::none);
        EXPECT_TRUE(fs.permissions() == melon::perms::unknown);
    }
    {
        melon::file_status fs{melon::file_type::regular};
        EXPECT_TRUE(fs.type() == melon::file_type::regular);
        EXPECT_TRUE(fs.permissions() == melon::perms::unknown);
    }
    {
        melon::file_status
                fs{melon::file_type::directory, melon::perms::owner_read | melon::perms::owner_write | melon::perms::owner_exec};
        EXPECT_TRUE(fs.type() == melon::file_type::directory);
        EXPECT_TRUE(fs.permissions() == melon::perms::owner_all);
        fs.type(melon::file_type::block);
        EXPECT_TRUE(fs.type() == melon::file_type::block);
        fs.type(melon::file_type::character);
        EXPECT_TRUE(fs.type() == melon::file_type::character);
        fs.type(melon::file_type::fifo);
        EXPECT_TRUE(fs.type() == melon::file_type::fifo);
        fs.type(melon::file_type::symlink);
        EXPECT_TRUE(fs.type()
                    == melon::file_type::symlink);
        fs.type(melon::file_type::socket);
        EXPECT_TRUE(fs.type() == melon::file_type::socket);
        fs.permissions(fs.permissions()
                       | melon::perms::group_all | melon::perms::others_all);
        EXPECT_TRUE(fs.permissions()
                    == melon::perms::all);
    }
    {
        melon::file_status fst(melon::file_type::regular);
        melon::file_status fs(std::move(fst));
        EXPECT_TRUE(fs.type()
                    == melon::file_type::regular);
        EXPECT_TRUE(fs.permissions()
                    == melon::perms::unknown);
    }
}
