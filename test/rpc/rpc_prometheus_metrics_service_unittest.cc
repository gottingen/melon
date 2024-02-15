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


// Date: 2023/05/06 15:10:00

#include <gtest/gtest.h>

#include "melon/utility/strings/string_piece.h"
#include "melon/utility/iobuf.h"
#include "melon/builtin/prometheus_metrics_service.h"

namespace {

class PrometheusMetricsDumperTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(PrometheusMetricsDumperTest, GetMetricsName) {
  EXPECT_EQ("", melon::GetMetricsName(""));

  EXPECT_EQ("commit_count", melon::GetMetricsName("commit_count"));

  EXPECT_EQ("commit_count", melon::GetMetricsName("commit_count{region=\"1000\"}"));
}

}
