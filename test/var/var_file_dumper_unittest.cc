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


// Date: 2015/08/27 17:12:38

#include "melon/var/reducer.h"
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
