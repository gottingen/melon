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




// Date: 2019/04/16 23:41:04

#include <gtest/gtest.h>
#include <melon/rpc/adaptive_max_concurrency.h>
#include <melon/rpc/adaptive_protocol_type.h>
#include <melon/rpc/adaptive_connection_type.h>

const std::string kAutoCL = "aUto";
const std::string kHttp = "hTTp";
const std::string kPooled = "PoOled";

TEST(AdaptiveMaxConcurrencyTest, ShouldConvertCorrectly) {
    melon::AdaptiveMaxConcurrency amc(0);

    EXPECT_EQ(melon::AdaptiveMaxConcurrency::UNLIMITED(), amc.type());
    EXPECT_EQ(melon::AdaptiveMaxConcurrency::UNLIMITED(), amc.value());
    EXPECT_EQ(0, int(amc));
    EXPECT_TRUE(amc == melon::AdaptiveMaxConcurrency::UNLIMITED());

    amc = 10;
    EXPECT_EQ(melon::AdaptiveMaxConcurrency::CONSTANT(), amc.type());
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
    melon::AdaptiveProtocolType apt;

    apt = kHttp;
    EXPECT_EQ(apt, melon::ProtocolType::PROTOCOL_HTTP);
    EXPECT_NE(apt, melon::ProtocolType::PROTOCOL_MELON_STD);

    apt = melon::ProtocolType::PROTOCOL_HTTP;
    EXPECT_EQ(apt, melon::ProtocolType::PROTOCOL_HTTP);
    EXPECT_NE(apt, melon::ProtocolType::PROTOCOL_MELON_STD);
}

TEST(AdaptiveConnectionTypeTest, ShouldConvertCorrectly) {
    melon::AdaptiveConnectionType act;

    act = melon::ConnectionType::CONNECTION_TYPE_POOLED;
    EXPECT_EQ(act, melon::ConnectionType::CONNECTION_TYPE_POOLED);
    EXPECT_NE(act, melon::ConnectionType::CONNECTION_TYPE_SINGLE);

    act = kPooled;
    EXPECT_EQ(act, melon::ConnectionType::CONNECTION_TYPE_POOLED);
    EXPECT_NE(act, melon::ConnectionType::CONNECTION_TYPE_SINGLE);
}

