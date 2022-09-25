
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "testing/gtest_wrap.h"
#include "melon/base/errno.h"
#include "melon/base/endpoint.h"
#include "melon/log/logging.h"
#include "melon/container/flat_map.h"

namespace {

    TEST(EndPointTest, comparisons) {
        melon::base::end_point p1(melon::base::int2ip(1234), 5678);
        melon::base::end_point p2 = p1;
        ASSERT_TRUE(p1 == p2 && !(p1 != p2));
        ASSERT_TRUE(p1 <= p2 && p1 >= p2 && !(p1 < p2 || p1 > p2));
        ++p2.port;
        ASSERT_TRUE(p1 != p2 && !(p1 == p2));
        ASSERT_TRUE(p1 < p2 && p2 > p1 && !(p2 <= p1 || p1 >= p2));
        --p2.port;
        p2.ip = melon::base::int2ip(melon::base::ip2int(p2.ip) - 1);
        ASSERT_TRUE(p1 != p2 && !(p1 == p2));
        ASSERT_TRUE(p1 > p2 && p2 < p1 && !(p1 <= p2 || p2 >= p1));
    }

    TEST(EndPointTest, ip_t) {
        MELON_LOG(INFO) << "INET_ADDRSTRLEN = " << INET_ADDRSTRLEN;

        melon::base::ip_t ip0;
        ASSERT_EQ(0, melon::base::str2ip("1.1.1.1", &ip0));
        ASSERT_STREQ("1.1.1.1", melon::base::ip2str(ip0).c_str());
        ASSERT_EQ(-1, melon::base::str2ip("301.1.1.1", &ip0));
        ASSERT_EQ(-1, melon::base::str2ip("1.-1.1.1", &ip0));
        ASSERT_EQ(-1, melon::base::str2ip("1.1.-101.1", &ip0));
        ASSERT_STREQ("1.0.0.0", melon::base::ip2str(melon::base::int2ip(1)).c_str());

        melon::base::ip_t ip1, ip2, ip3;
        ASSERT_EQ(0, melon::base::str2ip("192.168.0.1", &ip1));
        ASSERT_EQ(0, melon::base::str2ip("192.168.0.2", &ip2));
        ip3 = ip1;
        ASSERT_LT(ip1, ip2);
        ASSERT_LE(ip1, ip2);
        ASSERT_GT(ip2, ip1);
        ASSERT_GE(ip2, ip1);
        ASSERT_TRUE(ip1 != ip2);
        ASSERT_FALSE(ip1 == ip2);
        ASSERT_TRUE(ip1 == ip3);
        ASSERT_FALSE(ip1 != ip3);
    }

    TEST(EndPointTest, show_local_info) {
        MELON_LOG(INFO) << "my_ip is " << melon::base::my_ip() << std::endl
                  << "my_ip_cstr is " << melon::base::my_ip_cstr() << std::endl
                  << "my_hostname is " << melon::base::my_hostname();
    }

    TEST(EndPointTest, endpoint) {
        melon::base::end_point p1;
        ASSERT_EQ(melon::base::IP_ANY, p1.ip);
        ASSERT_EQ(0, p1.port);

        melon::base::end_point p2(melon::base::IP_NONE, -1);
        ASSERT_EQ(melon::base::IP_NONE, p2.ip);
        ASSERT_EQ(-1, p2.port);

        melon::base::end_point p3;
        ASSERT_EQ(-1, melon::base::str2endpoint(" 127.0.0.1:-1", &p3));
        ASSERT_EQ(-1, melon::base::str2endpoint(" 127.0.0.1:65536", &p3));
        ASSERT_EQ(0, melon::base::str2endpoint(" 127.0.0.1:65535", &p3));
        ASSERT_EQ(0, melon::base::str2endpoint(" 127.0.0.1:0", &p3));

        melon::base::end_point p4;
        ASSERT_EQ(0, melon::base::str2endpoint(" 127.0.0.1: 289 ", &p4));
        ASSERT_STREQ("127.0.0.1", melon::base::ip2str(p4.ip).c_str());
        ASSERT_EQ(289, p4.port);

        melon::base::end_point p5;
        ASSERT_EQ(-1, hostname2endpoint("localhost:-1", &p5));
        ASSERT_EQ(-1, hostname2endpoint("localhost:65536", &p5));
        ASSERT_EQ(0, hostname2endpoint("localhost:65535", &p5)) << melon_error();
        ASSERT_EQ(0, hostname2endpoint("localhost:0", &p5));

    }

    TEST(EndPointTest, hash_table) {
        melon::container::hash_map<melon::base::end_point, int> m;
        melon::base::end_point ep1(melon::base::IP_ANY, 123);
        melon::base::end_point ep2(melon::base::IP_ANY, 456);
        ++m[ep1];
        ASSERT_TRUE(m.find(ep1) != m.end());
        ASSERT_EQ(1, m.find(ep1)->second);
        ASSERT_EQ(1u, m.size());

        ++m[ep1];
        ASSERT_TRUE(m.find(ep1) != m.end());
        ASSERT_EQ(2, m.find(ep1)->second);
        ASSERT_EQ(1u, m.size());

        ++m[ep2];
        ASSERT_TRUE(m.find(ep2) != m.end());
        ASSERT_EQ(1, m.find(ep2)->second);
        ASSERT_EQ(2u, m.size());
    }

    TEST(EndPointTest, flat_map) {
        melon::container::FlatMap<melon::base::end_point, int> m;
        ASSERT_EQ(0, m.init(1024));
        uint32_t port = 8088;

        melon::base::end_point ep1(melon::base::IP_ANY, port);
        melon::base::end_point ep2(melon::base::IP_ANY, port);
        ++m[ep1];
        ++m[ep2];
        ASSERT_EQ(1u, m.size());

        melon::base::ip_t ip_addr;
        melon::base::str2ip("10.10.10.10", &ip_addr);
        int ip = melon::base::ip2int(ip_addr);

        for (int i = 0; i < 1023; ++i) {
            melon::base::end_point ep(melon::base::int2ip(++ip), port);
            ++m[ep];
        }

        melon::container::BucketInfo info = m.bucket_info();
        MELON_LOG(INFO) << "bucket info max long=" << info.longest_length
                  << " avg=" << info.average_length << std::endl;
        ASSERT_LT(info.longest_length, 32ul) << "detect hash collision and it's too large.";
    }

} // end of namespace
