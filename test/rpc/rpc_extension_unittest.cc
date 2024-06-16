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




// Date: Sun Jul 13 15:04:18 CST 2014

#include <sys/types.h>
#include <sys/socket.h>
#include <map>
#include <gtest/gtest.h>
#include <melon/utility/time.h>
#include <melon/base/macros.h>
#include <melon/rpc/extension.h>

class ExtensionTest : public ::testing::Test{
protected:
    ExtensionTest(){
    };
    virtual ~ExtensionTest(){};
    virtual void SetUp() {
    };
    virtual void TearDown() {
    };
};

inline melon::Extension<const int>* ConstIntExtension() {
    return melon::Extension<const int>::instance();
}

inline melon::Extension<int>* IntExtension() {
    return melon::Extension<int>::instance();
}


const int g_foo = 10;
const int g_bar = 20;
    
TEST_F(ExtensionTest, basic) {
    ConstIntExtension()->Register("foo", NULL);
    ConstIntExtension()->Register("foo", &g_foo);
    ConstIntExtension()->Register("bar", &g_bar);

    int* val1 = new int(0xbeef);
    int* val2 = new int(0xdead);
    ASSERT_EQ(0, IntExtension()->Register("hello", val1));
    ASSERT_EQ(-1, IntExtension()->Register("hello", val1));
    IntExtension()->Register("there", val2);
    ASSERT_EQ(val1, IntExtension()->Find("hello"));
    ASSERT_EQ(val2, IntExtension()->Find("there"));
    std::ostringstream os;
    IntExtension()->List(os, ':');
    ASSERT_EQ("hello:there", os.str());

    os.str("");
    ConstIntExtension()->List(os, ':');
    ASSERT_EQ("bar:foo", os.str());
}
