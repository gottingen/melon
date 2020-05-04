//
// Created by liyinbin on 2020/1/27.
//

#include <testing/filesystem_test_util.h>
#include <gtest/gtest.h>

TEST(TemporaryDirectory, fsTestTmpdir) {
    fs::path tempPath;
    {
        TemporaryDirectory t;
        tempPath = t.path();
        EXPECT_TRUE(fs::exists(fs::path(t.path()))
        );
        EXPECT_TRUE(fs::is_directory(t.path())
        );
    }
    EXPECT_TRUE(!
                        fs::exists(tempPath)
    );
}
