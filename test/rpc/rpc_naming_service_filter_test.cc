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

#include <stdio.h>
#include "testing/gtest_wrap.h"
#include <vector>
#include "melon/strings/str_format.h"
#include "melon/files/temp_file.h"
#include "melon/rpc/socket.h"
#include "melon/rpc/channel.h"
#include "melon/rpc/load_balancer.h"
#include "melon/rpc/policy/file_naming_service.h"

class NamingServiceFilterTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
}; 

class MyNSFilter: public melon::rpc::NamingServiceFilter {
public:
    bool Accept(const melon::rpc::ServerNode& node) const {
        return node.tag == "enable";
    }
};

TEST_F(NamingServiceFilterTest, sanity) {
    std::vector<melon::rpc::ServerNode> servers;
    const char *address_list[] =  {
        "10.127.0.1:1234",
        "10.128.0.1:1234 enable",
        "10.129.0.1:1234",
        "localhost:1234",
        "baidu.com:1234"
    };
    melon::temp_file tmp_file;
    {
        FILE* fp = fopen(tmp_file.fname(), "w");
        for (size_t i = 0; i < MELON_ARRAY_SIZE(address_list); ++i) {
            ASSERT_TRUE(fprintf(fp, "%s\n", address_list[i]));
        }
        fclose(fp);
    }
    MyNSFilter ns_filter;
    melon::rpc::Channel channel;
    melon::rpc::ChannelOptions opt;
    opt.ns_filter = &ns_filter;
    std::string ns = std::string("file://") + tmp_file.fname();
    ASSERT_EQ(0, channel.Init(ns.c_str(), "rr", &opt));

    melon::base::end_point ep;
    ASSERT_EQ(0, melon::base::hostname2endpoint("10.128.0.1:1234", &ep));
    for (int i = 0; i < 10; ++i) {
        melon::rpc::SocketUniquePtr tmp_sock;
        melon::rpc::LoadBalancer::SelectIn sel_in = { 0, false, false, 0, NULL };
        melon::rpc::LoadBalancer::SelectOut sel_out(&tmp_sock);
        ASSERT_EQ(0, channel._lb->SelectServer(sel_in, &sel_out));
        ASSERT_EQ(ep, tmp_sock->remote_side());
    }
}
