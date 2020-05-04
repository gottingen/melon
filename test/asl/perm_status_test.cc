//
// Created by liyinbin on 2020/1/27.
//

#include <testing/filesystem_test_util.h>
#include <gtest/gtest.h>

TEST(perm, all) {
    EXPECT_TRUE((fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec) == fs::perms::owner_all);
    EXPECT_TRUE((fs::perms::group_read | fs::perms::group_write | fs::perms::group_exec) == fs::perms::group_all);
    EXPECT_TRUE((fs::perms::others_read | fs::perms::others_write | fs::perms::others_exec) == fs::perms::others_all);
    EXPECT_TRUE((fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all) == fs::perms::all);
    EXPECT_TRUE((fs::perms::all | fs::perms::set_uid | fs::perms::set_gid | fs::perms::sticky_bit) == fs::perms::mask);
}

TEST(file_status, all) {
    {
        fs::file_status fs;
        EXPECT_TRUE(fs.type() == fs::file_type::none);
        EXPECT_TRUE(fs.permissions() == fs::perms::unknown);
    }
    {
        fs::file_status fs{fs::file_type::regular};
        EXPECT_TRUE(fs.type() == fs::file_type::regular);
        EXPECT_TRUE(fs.permissions() == fs::perms::unknown);
    }
    {
        fs::file_status
                fs{fs::file_type::directory, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec};
        EXPECT_TRUE(fs.type() == fs::file_type::directory);
        EXPECT_TRUE(fs.permissions() == fs::perms::owner_all);
        fs.type(fs::file_type::block);
        EXPECT_TRUE(fs.type() == fs::file_type::block);
        fs.type(fs::file_type::character);
        EXPECT_TRUE(fs.type() == fs::file_type::character);
        fs.type(fs::file_type::fifo);
        EXPECT_TRUE(fs.type() == fs::file_type::fifo);
        fs.type(fs::file_type::symlink);
        EXPECT_TRUE(fs.type()
                    == fs::file_type::symlink);
        fs.type(fs::file_type::socket);
        EXPECT_TRUE(fs.type() == fs::file_type::socket);
        fs.permissions(fs.permissions()
                       | fs::perms::group_all | fs::perms::others_all);
        EXPECT_TRUE(fs.permissions()
                    == fs::perms::all);
    }
    {
        fs::file_status fst(fs::file_type::regular);
        fs::file_status fs(std::move(fst));
        EXPECT_TRUE(fs.type()
                    == fs::file_type::regular);
        EXPECT_TRUE(fs.permissions()
                    == fs::perms::unknown);
    }
}
