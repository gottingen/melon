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




#include <gtest/gtest.h>
#include <melon/rpc/server.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/controller.h>
#include <melon/utility/strings/string_piece.h>
#include "echo.pb.h"

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class DummyEchoServiceImpl : public test::EchoService {
public:
    virtual ~DummyEchoServiceImpl() {}
    virtual void Echo(google::protobuf::RpcController* cntl_base,
                      const test::EchoRequest* request,
                      test::EchoResponse* response,
                      google::protobuf::Closure* done) {
        melon::ClosureGuard done_guard(done);
        return;
    }
};

enum STATE {
    HELP = 0,
    TYPE,
    GAUGE,
    SUMMARY
};

TEST(PrometheusMetrics, sanity) {
    melon::Server server;
    DummyEchoServiceImpl echo_svc;
    ASSERT_EQ(0, server.AddService(&echo_svc, melon::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start("127.0.0.1:8614", NULL));

    melon::Server server2;
    DummyEchoServiceImpl echo_svc2;
    ASSERT_EQ(0, server2.AddService(&echo_svc2, melon::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server2.Start("127.0.0.1:8615", NULL));

    melon::Channel channel;
    melon::ChannelOptions channel_opts;
    channel_opts.protocol = "http";
    ASSERT_EQ(0, channel.Init("127.0.0.1:8614", &channel_opts));
    melon::Controller cntl;
    cntl.http_request().uri() = "/melon_metrics";
    channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
    ASSERT_FALSE(cntl.Failed());
    std::string res = cntl.response_attachment().to_string();

    size_t start_pos = 0;
    size_t end_pos = 0;
    STATE state = HELP;
    char name_help[128];
    char name_type[128];
    char type[16];
    int matched = 0;
    int gauge_num = 0;
    bool summary_sum_gathered = false;
    bool summary_count_gathered = false;
    bool has_ever_summary = false;
    bool has_ever_gauge = false;

    while ((end_pos = res.find('\n', start_pos)) != mutil::StringPiece::npos) {
        res[end_pos] = '\0';       // safe;
        switch (state) {
            case HELP:
                matched = sscanf(res.data() + start_pos, "# HELP %s", name_help);
                ASSERT_EQ(1, matched);
                state = TYPE;
                break;
            case TYPE:
                matched = sscanf(res.data() + start_pos, "# TYPE %s %s", name_type, type);
                ASSERT_EQ(2, matched);
                ASSERT_STREQ(name_type, name_help);
                if (strcmp(type, "gauge") == 0) {
                    state = GAUGE;
                } else if (strcmp(type, "summary") == 0) {
                    state = SUMMARY;
                } else {
                    ASSERT_TRUE(false);
                }
                break;
            case GAUGE:
                matched = sscanf(res.data() + start_pos, "%s %d", name_type, &gauge_num);
                ASSERT_EQ(2, matched);
                ASSERT_STREQ(name_type, name_help);
                state = HELP;
                has_ever_gauge = true;
                break;
            case SUMMARY:
                if (mutil::StringPiece(res.data() + start_pos, end_pos - start_pos).find("quantile=")
                        == mutil::StringPiece::npos) {
                    matched = sscanf(res.data() + start_pos, "%s %d", name_type, &gauge_num);
                    ASSERT_EQ(2, matched);
                    ASSERT_TRUE(strncmp(name_type, name_help, strlen(name_help)) == 0);
                    if (mutil::StringPiece(name_type).ends_with("_sum")) {
                        ASSERT_FALSE(summary_sum_gathered);
                        summary_sum_gathered = true;
                    } else if (mutil::StringPiece(name_type).ends_with("_count")) {
                        ASSERT_FALSE(summary_count_gathered);
                        summary_count_gathered = true;
                    } else {
                        ASSERT_TRUE(false);
                    }
                    if (summary_sum_gathered && summary_count_gathered) {
                        state = HELP;
                        summary_sum_gathered = false;
                        summary_count_gathered = false;
                        has_ever_summary = true;
                    }
                } // else find "quantile=", just break to next line
                break;
            default:
                ASSERT_TRUE(false);
                break;
        }
        start_pos = end_pos + 1;
    }
    ASSERT_TRUE(has_ever_gauge && has_ever_summary);
    ASSERT_EQ(0, server.Stop(0));
    ASSERT_EQ(0, server.Join());
}
