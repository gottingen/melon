//
// Created by liyinbin on 2021/4/19.
//

#include "abel/net/end_point.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <utility>

#include "gtest/gtest.h"
#include "abel/strings/format.h"
#include "abel/strings/numbers.h"

using namespace std::literals;

namespace abel {

    TEST(EndpointBuilder, Retrieve) {
        end_point_builder er;
        auto ep2 = end_point::from_ipv4("192.0.2.1", 5678);
        memcpy(er.addr(), ep2.get(), ep2.length());
        *er.length() = ep2.length();
        ASSERT_EQ("192.0.2.1:5678", er.build().to_string());
    }

    TEST(end_point, to_string) {
        ASSERT_EQ("192.0.2.1:5678", end_point::from_ipv4("192.0.2.1", 5678).to_string());
    }

    TEST(end_point, ToString2) {
        end_point ep;
        auto ep2 = end_point::from_ipv4("192.0.2.1", 5678);
        ep = ep2;
        ASSERT_EQ("192.0.2.1:5678", ep.to_string());
    }

    TEST(end_point, ToString3) {
        ASSERT_EQ("192.0.2.1:5678",
                  abel::format("{}", end_point::from_ipv4("192.0.2.1", 5678)));
    }

    TEST(end_point, MoveToEmpty) {
        end_point ep;
        auto ep2 = end_point::from_ipv4("192.0.2.1", 5678);
        ep = ep2;
        ASSERT_EQ("192.0.2.1:5678", ep.to_string());
    }

    TEST(end_point, MoveFromEmpty) {
        end_point ep;
        auto ep2 = end_point::from_ipv4("192.0.2.1", 5678);
        ep2 = std::move(ep);
        ASSERT_TRUE(ep2.empty());
    }

    TEST(end_point, from_string) {
        auto ep = end_point::from_string("192.0.2.1:5678");
        ASSERT_EQ("192.0.2.1:5678", ep.to_string());

        ep = end_point::from_string("[2001:db8:8714:3a90::12]:1234");
        ASSERT_EQ("[2001:db8:8714:3a90::12]:1234", ep.to_string());
    }

    TEST(end_point, EndpointCompare) {
        auto ep1 = end_point::from_string("192.0.2.1:5678");
        auto ep2 = end_point::from_string("192.0.2.1:5678");
        auto ep3 = end_point::from_string("192.0.2.1:9999");
        ASSERT_EQ(ep1, ep2);
        ASSERT_FALSE(ep1 == ep3);
    }

    TEST(end_point, EndpointCopy) {
        auto ep1 = end_point::from_string("192.0.2.1:5678");
        auto ep2 = ep1;
        end_point ep3;

        ep3 = ep1;
        ASSERT_EQ(ep1, ep2);
        ASSERT_EQ(ep1, ep3);
        ASSERT_EQ(ep2, ep3);
    }

    TEST(end_point, TryParse) {
        auto ep = end_point::from_string("192.0.2.1:5678");
        //ASSERT_TRUE(ep);
        ASSERT_EQ("192.0.2.1:5678", ep.to_string());

        ep = end_point::from_string("[2001:db8:8714:3a90::12]:1234");
       // ASSERT_TRUE(ep);
        ASSERT_EQ("[2001:db8:8714:3a90::12]:1234", ep.to_string());
    }

    TEST(end_point, TryParse2) {
        auto ep = end_point::from_ipv4("192.0.2.1:5678");
        ASSERT_TRUE(ep);
        ASSERT_EQ("192.0.2.1:5678", ep->to_string());

        ep = end_point::from_ipv6("[2001:db8:8714:3a90::12]:1234");
        ASSERT_TRUE(ep);
        ASSERT_EQ("[2001:db8:8714:3a90::12]:1234", ep->to_string());
    }

    TEST(end_point, TryParse3) {
        auto ep = end_point::from_ipv6("192.0.2.1:5678");
        ASSERT_FALSE(ep);
        ep = end_point::from_ipv4("[2001:db8:8714:3a90::12]:1234");
        ASSERT_FALSE(ep);
    }

    TEST(end_point, GetIpPortV4) {
        auto ep = end_point::from_string("192.0.2.1:5678");
        //ASSERT_TRUE(ep);
        ASSERT_EQ("192.0.2.1", ep.get_ip());
        ASSERT_EQ(5678, ep.get_port());
    }

    TEST(end_point, GetIpPortV6) {
        auto ep = end_point::from_string("[2001:db8:8714:3a90::12]:1234");
        //ASSERT_TRUE(ep);
        ASSERT_EQ("2001:db8:8714:3a90::12", ep.get_ip());
        ASSERT_EQ(1234, ep.get_port());
    }

}  // namespace abel
