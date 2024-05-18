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
#include "melon/utility/string_printf.h"

namespace {

class BaiduStringPrintfTest : public ::testing::Test{
protected:
    BaiduStringPrintfTest(){
    };
    virtual ~BaiduStringPrintfTest(){};
    virtual void SetUp() {
    };
    virtual void TearDown() {
    };
};

TEST_F(BaiduStringPrintfTest, sanity) {

    {
        std::string sth = mutil::string_printf("hello %d 124 %s", 1, "world");
        ASSERT_EQ("hello 1 124 world", sth);

        ASSERT_EQ("hello 1 124 world", mutil::string_printf(sth.size(), "hello %d 124 %s", 1, "world"));
        ASSERT_EQ("hello 1 124 world", mutil::string_printf(sth.size() - 1, "hello %d 124 %s", 1, "world"));
        ASSERT_EQ("hello 1 124 world", mutil::string_printf(sth.size() + 1, "hello %d 124 %s", 1, "world"));

    }
    {
        std::string sth;
        ASSERT_EQ(0, mutil::string_printf(&sth, "boolean %d", 1));
        ASSERT_EQ("boolean 1", sth);
        ASSERT_EQ(0, mutil::string_appendf(&sth, "too simple"));
        ASSERT_EQ("boolean 1too simple", sth);
    }
}

}