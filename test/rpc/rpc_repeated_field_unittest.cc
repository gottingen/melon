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


#include "repeated.pb.h"
#include <gtest/gtest.h>
#include <melon/json2pb/pb_to_json.h>

class RepeatedFieldTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(RepeatedFieldTest, empty_array) {
    RepeatedMessage m;
    std::string json;

    ASSERT_TRUE(json2pb::ProtoMessageToJson(m, &json));
    std::cout << json << std::endl;

    m.add_strings();
    m.add_ints(1);
    m.add_msgs();
    json.clear();
    ASSERT_TRUE(json2pb::ProtoMessageToJson(m, &json));
    std::cout << json << std::endl;
}
