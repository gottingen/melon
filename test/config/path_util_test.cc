//

#include <abel/config/internal/path_util.h>

#include <gtest/gtest.h>

namespace {

    namespace flags = abel::flags_internal;

    TEST(FlagsPathUtilTest, TestBasename) {
        EXPECT_EQ(flags::Basename(""), "");
        EXPECT_EQ(flags::Basename("a.cc"), "a.cc");
        EXPECT_EQ(flags::Basename("dir/a.cc"), "a.cc");
        EXPECT_EQ(flags::Basename("dir1/dir2/a.cc"), "a.cc");
        EXPECT_EQ(flags::Basename("../dir1/dir2/a.cc"), "a.cc");
        EXPECT_EQ(flags::Basename("/dir1/dir2/a.cc"), "a.cc");
        EXPECT_EQ(flags::Basename("/dir1/dir2/../dir3/a.cc"), "a.cc");
    }

// --------------------------------------------------------------------

    TEST(FlagsPathUtilTest, TestPackage) {
        EXPECT_EQ(flags::Package(""), "");
        EXPECT_EQ(flags::Package("a.cc"), "");
        EXPECT_EQ(flags::Package("dir/a.cc"), "dir/");
        EXPECT_EQ(flags::Package("dir1/dir2/a.cc"), "dir1/dir2/");
        EXPECT_EQ(flags::Package("../dir1/dir2/a.cc"), "../dir1/dir2/");
        EXPECT_EQ(flags::Package("/dir1/dir2/a.cc"), "/dir1/dir2/");
        EXPECT_EQ(flags::Package("/dir1/dir2/../dir3/a.cc"), "/dir1/dir2/../dir3/");
    }

}  // namespace
