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


// Date: 2023/05/06 15:10:00

#include <gtest/gtest.h>

#include <melon/utility/strings/string_piece.h>
#include <melon/base/iobuf.h>
#include <melon/builtin/prometheus_metrics_service.h>

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
