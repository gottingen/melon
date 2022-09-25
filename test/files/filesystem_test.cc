
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <testing/filesystem_test_util.h>
#include "testing/gtest_wrap.h"

TEST(TemporaryDirectory, fsTestTmpdir) {
    melon::file_path tempPath;
    {
        TemporaryDirectory t;
        tempPath = t.path();
        EXPECT_TRUE(melon::exists(melon::file_path(t.path()))
        );
        EXPECT_TRUE(melon::is_directory(t.path())
        );
    }
    EXPECT_TRUE(!
                        melon::exists(tempPath)
    );
}
