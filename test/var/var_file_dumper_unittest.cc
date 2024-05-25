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


// Date: 2015/08/27 17:12:38

#include <melon/var/reducer.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <stdlib.h>

class FileDumperTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(FileDumperTest, filters) {
    melon::var::Adder<int> a1("a_latency");
    melon::var::Adder<int> a2("a_qps");
    melon::var::Adder<int> a3("a_error");
    melon::var::Adder<int> a4("process_*");
    melon::var::Adder<int> a5("default");
    google::SetCommandLineOption("var_dump_interval", "1");
    google::SetCommandLineOption("var_dump", "true");
    sleep(2);
}
