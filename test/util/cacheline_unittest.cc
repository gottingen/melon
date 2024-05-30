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


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <gtest/gtest.h>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>

namespace {

class CachelineTest : public testing::Test {
};

struct MELON_CACHELINE_ALIGNMENT Bar {
    int y;
};

struct Foo {
    char dummy1[0];
    int z;
    MELON_CACHELINE_ALIGNMENT int x[0];
    int y;
    int m;
    Bar bar;
};

TEST_F(CachelineTest, cacheline_alignment) {
    ASSERT_EQ(64u, offsetof(Foo, x));
    ASSERT_EQ(64u, offsetof(Foo, y));
    ASSERT_EQ(68u, offsetof(Foo, m));
    ASSERT_EQ(128u, offsetof(Foo, bar));
    ASSERT_EQ(64u, sizeof(Bar));
    ASSERT_EQ(192u, sizeof(Foo));
}

}
