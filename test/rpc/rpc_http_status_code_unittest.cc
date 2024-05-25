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




// File: test_http_status_code.cpp
// Date: 2014/11/04 18:33:39

#include <gtest/gtest.h>
#include <melon/rpc/http/http_status_code.h>

class HttpStatusTest : public testing::Test {
    void SetUp() {}
    void TearDown() {}
};

TEST_F(HttpStatusTest, sanity) {
    ASSERT_STREQ("OK", melon::HttpReasonPhrase(
                     melon::HTTP_STATUS_OK));
    ASSERT_STREQ("Continue", melon::HttpReasonPhrase(
                     melon::HTTP_STATUS_CONTINUE));
    ASSERT_STREQ("HTTP Version Not Supported", melon::HttpReasonPhrase(
                     melon::HTTP_STATUS_VERSION_NOT_SUPPORTED));
    ASSERT_STREQ("Unknown status code (-2)", melon::HttpReasonPhrase(-2));
}
