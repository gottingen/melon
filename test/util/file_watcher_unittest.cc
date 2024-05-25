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
#include <melon/utility/files/file_watcher.h>
#include <melon/utility/logging.h>

namespace {
class FileWatcherTest : public ::testing::Test{
protected:
    FileWatcherTest(){};
    virtual ~FileWatcherTest(){};
    virtual void SetUp() {
    };
    virtual void TearDown() {
    };
};
 
//! gejun: check basic functions of mutil::FileWatcher
TEST_F(FileWatcherTest, random_op) {
    srand (time(0));
    
    mutil::FileWatcher fw;
    EXPECT_EQ (0, fw.init("dummy_file"));
    
    for (int i=0; i<30; ++i) {
        if (rand() % 2) {
            const mutil::FileWatcher::Change ret = fw.check_and_consume();
            switch (ret) {
            case mutil::FileWatcher::UPDATED:
                MLOG(INFO) << fw.filepath() << " is updated";
                break;
            case mutil::FileWatcher::CREATED:
                MLOG(INFO) << fw.filepath() << " is created";
                break;
            case mutil::FileWatcher::DELETED:
                MLOG(INFO) << fw.filepath() << " is deleted";
                break;
            case mutil::FileWatcher::UNCHANGED:
                MLOG(INFO) << fw.filepath() << " does not change or still not exist";
                break;
            }
        }
        
        switch (rand() % 2) {
        case 0:
            ASSERT_EQ(0, system("touch dummy_file"));
            MLOG(INFO) << "action: touch dummy_file";
            break;
        case 1:
            ASSERT_EQ(0, system("rm -f dummy_file"));
            MLOG(INFO) << "action: rm -f dummy_file";
            break;
        case 2:
            MLOG(INFO) << "action: (nothing)";
            break;
        }
        
        usleep (10000);
    }
    ASSERT_EQ(0, system("rm -f dummy_file"));
}

}  // namespace
