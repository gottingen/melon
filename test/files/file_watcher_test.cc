
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/gtest_wrap.h"
#include "melon/files/file_watcher.h"
#include "melon/log/logging.h"

namespace {
    class FileWatcherTest : public ::testing::Test {
    protected:
        FileWatcherTest() {};

        virtual ~FileWatcherTest() {};

        virtual void SetUp() {
        };

        virtual void TearDown() {
        };
    };

    /// check basic functions of melon::file_watcher
    TEST_F(FileWatcherTest, random_op) {
        srand(time(0));

        melon::file_watcher fw;
        EXPECT_EQ (0, fw.init("dummy_file"));

        for (int i = 0; i < 30; ++i) {
            if (rand() % 2) {
                const melon::file_watcher::Change ret = fw.check_and_consume();
                switch (ret) {
                    case melon::file_watcher::UPDATED:
                        MELON_LOG(INFO) << fw.filepath() << " is updated";
                        break;
                    case melon::file_watcher::CREATED:
                        MELON_LOG(INFO) << fw.filepath() << " is created";
                        break;
                    case melon::file_watcher::DELETED:
                        MELON_LOG(INFO) << fw.filepath() << " is deleted";
                        break;
                    case melon::file_watcher::UNCHANGED:
                        MELON_LOG(INFO) << fw.filepath() << " does not change or still not exist";
                        break;
                }
            }

            switch (rand() % 2) {
                case 0:
                    ASSERT_EQ(0, system("touch dummy_file"));
                    MELON_LOG(INFO) << "action: touch dummy_file";
                    break;
                case 1:
                    ASSERT_EQ(0, system("rm -f dummy_file"));
                    MELON_LOG(INFO) << "action: rm -f dummy_file";
                    break;
                case 2:
                    MELON_LOG(INFO) << "action: (nothing)";
                    break;
            }

            usleep(10000);
        }
        ASSERT_EQ(0, system("rm -f dummy_file"));
    }

}  // namespace
