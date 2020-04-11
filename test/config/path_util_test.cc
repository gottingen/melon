//

#include <abel/config/internal/path_util.h>

#include <gtest/gtest.h>

namespace {

    namespace flags = abel::flags_internal;

    TEST(FlagsPathUtilTest, TestBasename) {
        EXPECT_EQ(flags::base_name(""), "");
        EXPECT_EQ(flags::base_name("a.cc"), "a.cc");
        EXPECT_EQ(flags::base_name("dir/a.cc"), "a.cc");
        EXPECT_EQ(flags::base_name("dir1/dir2/a.cc"), "a.cc");
        EXPECT_EQ(flags::base_name("../dir1/dir2/a.cc"), "a.cc");
        EXPECT_EQ(flags::base_name("/dir1/dir2/a.cc"), "a.cc");
        EXPECT_EQ(flags::base_name("/dir1/dir2/../dir3/a.cc"), "a.cc");
    }

// --------------------------------------------------------------------

    TEST(FlagsPathUtilTest, TestPackage) {
        EXPECT_EQ(flags::package(""), "");
        EXPECT_EQ(flags::package("a.cc"), "");
        EXPECT_EQ(flags::package("dir/a.cc"), "dir/");
        EXPECT_EQ(flags::package("dir1/dir2/a.cc"), "dir1/dir2/");
        EXPECT_EQ(flags::package("../dir1/dir2/a.cc"), "../dir1/dir2/");
        EXPECT_EQ(flags::package("/dir1/dir2/a.cc"), "/dir1/dir2/");
        EXPECT_EQ(flags::package("/dir1/dir2/../dir3/a.cc"), "/dir1/dir2/../dir3/");
    }

}  // namespace
