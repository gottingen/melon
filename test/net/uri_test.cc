//
// Created by liyinbin on 2021/5/1.
//


#include "abel/net/uri.h"

#include <string>

#include "gtest/gtest.h"


namespace abel {

    TEST(Uri, Parse) {
        std::string uri_str =
                "http://www.baidu.com/"
                "s?tn=monline_dg&bs=DVLOG&f=8&wd=glog+DVLOG#fragment";
        auto parsed = parse_uri(uri_str);
        ASSERT_TRUE(parsed);

        EXPECT_EQ(uri_str, parsed->to_string());
        EXPECT_EQ("http", parsed->scheme());

        ASSERT_EQ("www.baidu.com", parsed->host());
        ASSERT_EQ(0, parsed->port());

        EXPECT_EQ("tn=monline_dg&bs=DVLOG&f=8&wd=glog+DVLOG", parsed->query());

        ASSERT_EQ("fragment", parsed->fragment());
        ASSERT_TRUE(parse_uri("http://q5(826753,65536)/monitro/es/dimeagg/"));

        ASSERT_EQ("monline_dg", parsed->get_query("tn"));
        ASSERT_EQ("DVLOG", parsed->get_query("bs"));
    }

    TEST(Uri, ParseAuthority) {
        std::string uristr =
                "http://username:password@127.0.0.1:8080/s?tn=monline_dg&bs=DVLOG";
        auto parsed = parse_uri(uristr);
        ASSERT_TRUE(parsed);
        EXPECT_EQ(uristr, parsed->to_string());
        EXPECT_EQ("http", parsed->scheme());

        ASSERT_EQ("/s", parsed->path());
        EXPECT_EQ("username:password", parsed->userinfo());
        EXPECT_EQ("127.0.0.1", parsed->host());
        EXPECT_EQ(8080, parsed->port());
    }

    TEST(Uri, ParseRelative) {
        const char *uristr = "/rpc?method=rpc_examples.EchoServer.Echo&format=json";
        auto parsed = parse_uri(uristr);
        ASSERT_TRUE(parsed);
        ASSERT_EQ("/rpc", parsed->path());
        ASSERT_EQ("method=rpc_examples.EchoServer.Echo&format=json", parsed->query());
    }

    TEST(Uri, BadUrl) {
        ASSERT_FALSE(parse_uri("http://^www.lianjiew.com/"));  // leading -
        ASSERT_FALSE(parse_uri("http://platform`info.py/"));  // domain contains _
        ASSERT_FALSE(parse_uri(" http://platform%info.py/"));  // leading space
    }

    TEST(Uri_builder, builder) {
        std::string uri_str = "http://username:password@127.0.0.1:8080/s?tn=monline_dg&bs=DVLOG";

        http_uri_builder builder;
        builder.set_http_url(uri_str);

        ASSERT_TRUE(builder.build(true));
        builder.remove_query("tn");
        builder.remove_query("bs");
        ASSERT_EQ(builder.to_string(true), "http://username:password@127.0.0.1:8080/s");
    }

}  // namespace abel
