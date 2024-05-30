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
#include <melon/utility/string_printf.h>

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
