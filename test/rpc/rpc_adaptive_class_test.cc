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



// Date: 2019/04/16 23:41:04

#include "testing/gtest_wrap.h"
#include "melon/rpc/adaptive_max_concurrency.h"
#include "melon/rpc/adaptive_protocol_type.h"
#include "melon/rpc/adaptive_connection_type.h"

const std::string kAutoCL = "aUto";
const std::string kHttp = "hTTp";
const std::string kPooled = "PoOled";

TEST(AdaptiveMaxConcurrencyTest, ShouldConvertCorrectly) {
    melon::rpc::AdaptiveMaxConcurrency amc(0);

    EXPECT_EQ(melon::rpc::AdaptiveMaxConcurrency::UNLIMITED(), amc.type());
    EXPECT_EQ(melon::rpc::AdaptiveMaxConcurrency::UNLIMITED(), amc.value());
    EXPECT_EQ(0, int(amc));
    EXPECT_TRUE(amc == melon::rpc::AdaptiveMaxConcurrency::UNLIMITED());

    amc = 10;
    EXPECT_EQ(melon::rpc::AdaptiveMaxConcurrency::CONSTANT(), amc.type());
    EXPECT_EQ("10", amc.value());
    EXPECT_EQ(10, int(amc));
    EXPECT_EQ(amc, "10");

    amc = kAutoCL;
    EXPECT_EQ(kAutoCL, amc.type());
    EXPECT_EQ(kAutoCL, amc.value());
    EXPECT_EQ(int(amc), -1);
    EXPECT_TRUE(amc == "auto");
}

TEST(AdaptiveProtocolTypeTest, ShouldConvertCorrectly) {
    melon::rpc::AdaptiveProtocolType apt;

    apt = kHttp;
    EXPECT_EQ(apt, melon::rpc::ProtocolType::PROTOCOL_HTTP);
    EXPECT_NE(apt, melon::rpc::ProtocolType::PROTOCOL_BAIDU_STD);

    apt = melon::rpc::ProtocolType::PROTOCOL_HTTP;
    EXPECT_EQ(apt, melon::rpc::ProtocolType::PROTOCOL_HTTP);
    EXPECT_NE(apt, melon::rpc::ProtocolType::PROTOCOL_BAIDU_STD);
}

TEST(AdaptiveConnectionTypeTest, ShouldConvertCorrectly) {
    melon::rpc::AdaptiveConnectionType act;

    act = melon::rpc::ConnectionType::CONNECTION_TYPE_POOLED;
    EXPECT_EQ(act, melon::rpc::ConnectionType::CONNECTION_TYPE_POOLED);
    EXPECT_NE(act, melon::rpc::ConnectionType::CONNECTION_TYPE_SINGLE);

    act = kPooled;
    EXPECT_EQ(act, melon::rpc::ConnectionType::CONNECTION_TYPE_POOLED);
    EXPECT_NE(act, melon::rpc::ConnectionType::CONNECTION_TYPE_SINGLE);
}

