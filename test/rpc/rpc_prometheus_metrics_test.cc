
#include "testing/gtest_wrap.h"
#include "melon/rpc/server.h"
#include "melon/rpc/channel.h"
#include "melon/rpc/controller.h"
#include "melon/strings/ends_with.h"
#include "melon/strings/str_split.h"
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
        melon::rpc::ClosureGuard done_guard(done);
        return;
    }
};

enum STATE {
    HELP = 0,
    TYPE,
    COUNTER,
    GAUGE,
    SUMMARY,
    HISTOGRAM
};

TEST(PrometheusMetrics, sanity) {
    melon::rpc::Server server;
    DummyEchoServiceImpl echo_svc;
    ASSERT_EQ(0, server.AddService(&echo_svc, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server.Start("127.0.0.1:8614", nullptr));

    melon::rpc::Server server2;
    DummyEchoServiceImpl echo_svc2;
    ASSERT_EQ(0, server2.AddService(&echo_svc2, melon::rpc::SERVER_DOESNT_OWN_SERVICE));
    ASSERT_EQ(0, server2.Start("127.0.0.1:8615", nullptr));

    melon::rpc::Channel channel;
    melon::rpc::ChannelOptions channel_opts;
    channel_opts.protocol = "http";
    ASSERT_EQ(0, channel.Init("127.0.0.1:8614", &channel_opts));
    melon::rpc::Controller cntl;
    cntl.http_request().uri() = "/melon_metrics";
    channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
    ASSERT_FALSE(cntl.Failed());
    std::string res = cntl.response_attachment().to_string();
    std::cout <<"res: "<< res << std::endl;
    size_t end_pos = 0;
    STATE state = HELP;
    char name_help[128];
    char name_type[128];
    char type[16];
    int matched = 0;
    int gauge_num = 0;
    bool summary_sum_gathered = false;
    bool summary_count_gathered = false;
    bool has_ever_summary_or_histogram = false;
    bool has_ever_gauge = false;
    bool has_ever_counter = false;

    std::vector<std::string> lines = melon::string_split(res, '\n');

    for(auto &item : lines) {
        res[end_pos] = '\0';       // safe;
        std::cout<<"precess: "<<item<<std::endl;
        std::cout<<"state: "<<state<<std::endl;
        if(item.empty()) {
            continue;
        }
        switch (state) {
            case HELP:
                matched = sscanf(item.c_str(), "# HELP %s", name_help);
                ASSERT_EQ(1, matched);
                state = TYPE;
                break;
            case TYPE:
                matched = sscanf(item.c_str(), "# TYPE %s %s", name_type, type);
                ASSERT_EQ(2, matched);
                ASSERT_STREQ(name_type, name_help);
                if (strcmp(type, "gauge") == 0) {
                    state = GAUGE;
                } else if (strcmp(type, "summary") == 0) {
                    state = SUMMARY;
                } else if (strcmp(type, "histogram") == 0){
                    state = HISTOGRAM;
                } else if (strcmp(type, "counter") == 0){
                    state = COUNTER;
                }else {
                    ASSERT_TRUE(false);
                }
                std::cout<<"type: "<<type<<" state: "<<state<<std::endl;
                break;
            case GAUGE:
                matched = sscanf(item.c_str(), "%s %d", name_type, &gauge_num);
                ASSERT_EQ(2, matched);
                ASSERT_STREQ(name_type, name_help);
                state = HELP;
                has_ever_gauge = true;
                break;
            case COUNTER:
                matched = sscanf(item.c_str(), "%s %d", name_type, &gauge_num);
                ASSERT_EQ(2, matched);
                ASSERT_STREQ(name_type, name_help);
                state = HELP;
                has_ever_counter = true;
                break;
            case HISTOGRAM:
                if (std::string_view(item).find("+Inf")
                    != std::string_view::npos) {
                    has_ever_summary_or_histogram = true;
                    state = HELP;
                }
                break;
            case SUMMARY:
                if (std::string_view(item).find("quantile=")
                        == std::string_view::npos) {
                    matched = sscanf(item.c_str(), "%s %d", name_type, &gauge_num);
                    ASSERT_EQ(2, matched);
                    ASSERT_TRUE(strncmp(name_type, name_help, strlen(name_help)) == 0);
                    if (melon::ends_with(name_type, "_sum")) {
                        ASSERT_FALSE(summary_sum_gathered);
                        summary_sum_gathered = true;
                    } else if (melon::ends_with(name_type, "_count")) {
                        ASSERT_FALSE(summary_count_gathered);
                        summary_count_gathered = true;
                    } else {
                        ASSERT_TRUE(false);
                    }
                    if (summary_sum_gathered && summary_count_gathered) {
                        state = HELP;
                        summary_sum_gathered = false;
                        summary_count_gathered = false;
                        has_ever_summary_or_histogram = true;
                    }
                } // else find "quantile=", just break to next line
                break;
            default:
                ASSERT_TRUE(false);
                break;
        }
    }
    ASSERT_TRUE(has_ever_gauge);
    ASSERT_TRUE(has_ever_counter);
    ASSERT_TRUE(has_ever_summary_or_histogram);
    ASSERT_EQ(0, server.Stop(0));
    ASSERT_EQ(0, server.Join());
}
