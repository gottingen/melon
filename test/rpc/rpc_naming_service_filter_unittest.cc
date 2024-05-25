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


#include <stdio.h>
#include <gtest/gtest.h>
#include <vector>
#include <melon/utility/string_printf.h>
#include "melon/utility/files/temp_file.h"
#include <melon/rpc/socket.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/load_balancer.h>
#include <melon/naming/file_naming_service.h>

class NamingServiceFilterTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
}; 

class MyNSFilter: public melon::NamingServiceFilter {
public:
    bool Accept(const melon::ServerNode& node) const {
        return node.tag == "enable";
    }
};

TEST_F(NamingServiceFilterTest, sanity) {
    std::vector<melon::ServerNode> servers;
    const char *address_list[] =  {
        "10.127.0.1:1234",
        "10.128.0.1:1234 enable",
        "10.129.0.1:1234",
        "localhost:1234",
        "baidu.com:1234"
    };
    mutil::TempFile tmp_file;
    {
        FILE* fp = fopen(tmp_file.fname(), "w");
        for (size_t i = 0; i < ARRAY_SIZE(address_list); ++i) {
            ASSERT_TRUE(fprintf(fp, "%s\n", address_list[i]));
        }
        fclose(fp);
    }
    MyNSFilter ns_filter;
    melon::Channel channel;
    melon::ChannelOptions opt;
    opt.ns_filter = &ns_filter;
    std::string ns = std::string("file://") + tmp_file.fname();
    ASSERT_EQ(0, channel.Init(ns.c_str(), "rr", &opt));

    mutil::EndPoint ep;
    ASSERT_EQ(0, mutil::hostname2endpoint("10.128.0.1:1234", &ep));
    for (int i = 0; i < 10; ++i) {
        melon::SocketUniquePtr tmp_sock;
        melon::LoadBalancer::SelectIn sel_in = { 0, false, false, 0, NULL };
        melon::LoadBalancer::SelectOut sel_out(&tmp_sock);
        ASSERT_EQ(0, channel._lb->SelectServer(sel_in, &sel_out));
        ASSERT_EQ(ep, tmp_sock->remote_side());
    }
}
