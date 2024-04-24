// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#include <gtest/gtest.h>
#include "melon/utility/files/file_watcher.h"
#include "melon/utility/logging.h"

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
