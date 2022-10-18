// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.



// File: test_http_status_code.cpp
// Date: 2014/11/04 18:33:39

#include "testing/gtest_wrap.h"
#include "melon/rpc/http_status_code.h"

class HttpStatusTest : public testing::Test {
    void SetUp() {}
    void TearDown() {}
};

TEST_F(HttpStatusTest, sanity) {
    ASSERT_STREQ("OK", melon::rpc::HttpReasonPhrase(
                     melon::rpc::HTTP_STATUS_OK));
    ASSERT_STREQ("Continue", melon::rpc::HttpReasonPhrase(
                     melon::rpc::HTTP_STATUS_CONTINUE));
    ASSERT_STREQ("HTTP Version Not Supported", melon::rpc::HttpReasonPhrase(
                     melon::rpc::HTTP_STATUS_VERSION_NOT_SUPPORTED));
    ASSERT_STREQ("Unknown status code (-2)", melon::rpc::HttpReasonPhrase(-2));
}
