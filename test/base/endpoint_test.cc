
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "testing/gtest_wrap.h"
#include "melon/base/errno.h"
#include "melon/base/endpoint.h"
#include "melon/base/endpoint_extended.h"
#include "melon/log/logging.h"
#include "melon/container/flat_map.h"

namespace {

    using melon::detail::extended_end_point;

    TEST(EndPointTest, comparisons) {
        melon::end_point p1(melon::int2ip(1234), 5678);
        melon::end_point p2 = p1;
        ASSERT_TRUE(p1 == p2 && !(p1 != p2));
        ASSERT_TRUE(p1 <= p2 && p1 >= p2 && !(p1 < p2 || p1 > p2));
        ++p2.port;
        ASSERT_TRUE(p1 != p2 && !(p1 == p2));
        ASSERT_TRUE(p1 < p2 && p2 > p1 && !(p2 <= p1 || p1 >= p2));
        --p2.port;
        p2.ip = melon::int2ip(melon::ip2int(p2.ip) - 1);
        ASSERT_TRUE(p1 != p2 && !(p1 == p2));
        ASSERT_TRUE(p1 > p2 && p2 < p1 && !(p1 <= p2 || p2 >= p1));
    }

    TEST(EndPointTest, ip_t) {
        MELON_LOG(INFO) << "INET_ADDRSTRLEN = " << INET_ADDRSTRLEN;

        melon::ip_t ip0;
        ASSERT_EQ(0, melon::str2ip("1.1.1.1", &ip0));
        ASSERT_STREQ("1.1.1.1", melon::ip2str(ip0).c_str());
        ASSERT_EQ(-1, melon::str2ip("301.1.1.1", &ip0));
        ASSERT_EQ(-1, melon::str2ip("1.-1.1.1", &ip0));
        ASSERT_EQ(-1, melon::str2ip("1.1.-101.1", &ip0));
        ASSERT_STREQ("1.0.0.0", melon::ip2str(melon::int2ip(1)).c_str());

        melon::ip_t ip1, ip2, ip3;
        ASSERT_EQ(0, melon::str2ip("192.168.0.1", &ip1));
        ASSERT_EQ(0, melon::str2ip("192.168.0.2", &ip2));
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
        MELON_LOG(INFO) << "my_ip is " << melon::my_ip() << std::endl
                        << "my_ip_cstr is " << melon::my_ip_cstr() << std::endl
                        << "my_hostname is " << melon::my_hostname();
    }

    TEST(EndPointTest, endpoint) {
        melon::end_point p1;
        ASSERT_EQ(melon::IP_ANY, p1.ip);
        ASSERT_EQ(0, p1.port);

        melon::end_point p2(melon::IP_NONE, -1);
        ASSERT_EQ(melon::IP_NONE, p2.ip);
        ASSERT_EQ(-1, p2.port);

        melon::end_point p3;
        ASSERT_EQ(-1, melon::str2endpoint(" 127.0.0.1:-1", &p3));
        ASSERT_EQ(-1, melon::str2endpoint(" 127.0.0.1:65536", &p3));
        ASSERT_EQ(0, melon::str2endpoint(" 127.0.0.1:65535", &p3));
        ASSERT_EQ(0, melon::str2endpoint(" 127.0.0.1:0", &p3));

        melon::end_point p4;
        ASSERT_EQ(0, melon::str2endpoint(" 127.0.0.1: 289 ", &p4));
        ASSERT_STREQ("127.0.0.1", melon::ip2str(p4.ip).c_str());
        ASSERT_EQ(289, p4.port);

        melon::end_point p5;
        ASSERT_EQ(-1, hostname2endpoint("localhost:-1", &p5));
        ASSERT_EQ(-1, hostname2endpoint("localhost:65536", &p5));
        ASSERT_EQ(0, hostname2endpoint("localhost:65535", &p5)) << melon_error();
        ASSERT_EQ(0, hostname2endpoint("localhost:0", &p5));

    }

    TEST(EndPointTest, hash_table) {
        melon::container::hash_map<melon::end_point, int> m;
        melon::end_point ep1(melon::IP_ANY, 123);
        melon::end_point ep2(melon::IP_ANY, 456);
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
        melon::container::FlatMap<melon::end_point, int> m;
        ASSERT_EQ(0, m.init(1024));
        uint32_t port = 8088;

        melon::end_point ep1(melon::IP_ANY, port);
        melon::end_point ep2(melon::IP_ANY, port);
        ++m[ep1];
        ++m[ep2];
        ASSERT_EQ(1u, m.size());

        melon::ip_t ip_addr;
        melon::str2ip("10.10.10.10", &ip_addr);
        int ip = melon::ip2int(ip_addr);

        for (int i = 0; i < 1023; ++i) {
            melon::end_point ep(melon::int2ip(++ip), port);
            ++m[ep];
        }

        melon::container::BucketInfo info = m.bucket_info();
        MELON_LOG(INFO) << "bucket info max long=" << info.longest_length
                        << " avg=" << info.average_length << std::endl;
        ASSERT_LT(info.longest_length, 32ul) << "detect hash collision and it's too large.";
    }


    void *server_proc(void *arg) {
        int listen_fd = (int64_t) arg;
        sockaddr_storage ss;
        socklen_t len = sizeof(ss);
        int fd = accept(listen_fd, (sockaddr *) &ss, &len);
        if (fd > 0) {
            close(fd);
        }
        return (void *) (int64_t) fd;
    }

    static void test_listen_connect(const std::string &server_addr, const std::string &exp_client_addr) {
        melon::end_point point;
        ASSERT_EQ(0, melon::str2endpoint(server_addr.c_str(), &point));
        ASSERT_EQ(server_addr, melon::endpoint2str(point).c_str());

        int listen_fd = melon::tcp_listen(point);
        ASSERT_GT(listen_fd, 0);
        pthread_t pid;
        pthread_create(&pid, NULL, server_proc, (void *) (int64_t) listen_fd);

        int fd = melon::tcp_connect(point, NULL);
        ASSERT_GT(fd, 0);
        melon::end_point point2;
        ASSERT_EQ(0, melon::get_local_side(fd, &point2));

        std::string s = melon::endpoint2str(point2).c_str();
        if (melon::get_endpoint_type(point2) == AF_UNIX) {
            ASSERT_EQ(exp_client_addr, s);
        } else {
            ASSERT_EQ(exp_client_addr, s.substr(0, exp_client_addr.size()));
        }
        ASSERT_EQ(0, melon::get_remote_side(fd, &point2));
        ASSERT_EQ(server_addr, melon::endpoint2str(point2).c_str());
        close(fd);

        void *ret = nullptr;
        pthread_join(pid, &ret);
        ASSERT_GT((int64_t) ret, 0);
        close(listen_fd);
    }

    static void test_parse_and_serialize(const std::string &instr, const std::string &outstr) {
        melon::end_point ep;
        ASSERT_EQ(0, melon::str2endpoint(instr.c_str(), &ep));
        melon::end_point_str s = melon::endpoint2str(ep);
        ASSERT_EQ(outstr, std::string(s.c_str()));
    }

    TEST(EndPointTest, ipv4) {
        test_listen_connect("127.0.0.1:8787", "127.0.0.1:");
    }

    TEST(EndPointTest, ipv6) {
        // FIXME: test environ may not support ipv6
        // test_listen_connect("[::1]:8787", "[::1]:");

        test_parse_and_serialize("[::1]:8080", "[::1]:8080");
        test_parse_and_serialize("  [::1]:65535  ", "[::1]:65535");
        test_parse_and_serialize("  [2001:0db8:a001:0002:0003:0ab9:C0A8:0102]:65535  ",
                                 "[2001:db8:a001:2:3:ab9:c0a8:102]:65535");

        melon::end_point ep;
        ASSERT_EQ(-1, melon::str2endpoint("[2001:db8:1:2:3:ab9:c0a8:102]", &ep));
        ASSERT_EQ(-1, melon::str2endpoint("[2001:db8:1:2:3:ab9:c0a8:102]#654321", &ep));
        ASSERT_EQ(-1, melon::str2endpoint("ipv6:2001:db8:1:2:3:ab9:c0a8:102", &ep));
        ASSERT_EQ(-1, melon::str2endpoint("[", &ep));
        ASSERT_EQ(-1, melon::str2endpoint("[::1", &ep));
        ASSERT_EQ(-1, melon::str2endpoint("[]:80", &ep));
        ASSERT_EQ(-1, melon::str2endpoint("[]", &ep));
        ASSERT_EQ(-1, melon::str2endpoint("[]:", &ep));
    }

    TEST(EndPointTest, unix_socket) {
        ::unlink("test.sock");
        test_listen_connect("unix:test.sock", "unix:");
        ::unlink("test.sock");

        melon::end_point point;
        ASSERT_EQ(-1, melon::str2endpoint("", &point));
        ASSERT_EQ(-1, melon::str2endpoint("a.sock", &point));
        ASSERT_EQ(-1, melon::str2endpoint("unix:", &point));
        ASSERT_EQ(-1, melon::str2endpoint(" unix: ", &point));
        ASSERT_EQ(0, melon::str2endpoint("unix://a.sock", 123, &point));
        ASSERT_EQ(std::string("unix://a.sock"), melon::endpoint2str(point).c_str());
        ASSERT_EQ(-1, melon::str2endpoint("unix:tooloooooooooooooooooooooooooooooooooooooooooooooooooo"
                                          "ooooooooooooooooooooooooooooooooooooooooooooooong.sock", &point));
        ASSERT_EQ(0, melon::str2endpoint(" unix:loooooooooooooooooooooooooooooooooooooooooooooooooooo"
                                         "ooooooooooooooooooooooooooooooooooooooooooooooong.sock", &point));
        ASSERT_EQ(std::string("unix:loooooooooooooooooooooooooooooooooooooooooooooooooooo"
                              "ooooooooooooooooooooooooooooooooooooooooooooooong.sock"),
                  melon::endpoint2str(point).c_str());
        char buf[128] = {0}; // braft use this size of buffer
        size_t ret = snprintf(buf, sizeof(buf), "%s:%d", melon::endpoint2str(point).c_str(), INT_MAX);
        ASSERT_LT(ret, sizeof(buf) - 1);
    }

    TEST(EndPointTest, original_endpoint) {
        melon::end_point ep;
        ASSERT_FALSE(extended_end_point::is_extended(ep));
        ASSERT_EQ(NULL, extended_end_point::address(ep));

        ASSERT_EQ(0, melon::str2endpoint("1.2.3.4:5678", &ep));
        ASSERT_FALSE(extended_end_point::is_extended(ep));
        ASSERT_EQ(NULL, extended_end_point::address(ep));

        // ctor & dtor
        {
            melon::end_point ep2(ep);
            ASSERT_FALSE(extended_end_point::is_extended(ep));
            ASSERT_EQ(ep.ip, ep2.ip);
            ASSERT_EQ(ep.port, ep2.port);
        }

        // assign
        melon::end_point ep2;
        ep2 = ep;
        ASSERT_EQ(ep.ip, ep2.ip);
        ASSERT_EQ(ep.port, ep2.port);
    }

    TEST(EndPointTest, extended_endpoint) {
        melon::end_point ep;
        ASSERT_EQ(0, melon::str2endpoint("unix:sock.file", &ep));
        ASSERT_TRUE(extended_end_point::is_extended(ep));
        extended_end_point *eep = extended_end_point::address(ep);
        ASSERT_TRUE(eep);
        ASSERT_EQ(AF_UNIX, eep->family());
        ASSERT_EQ(1, eep->_ref_count.load());

        // copy ctor & dtor
        {
            melon::end_point tmp(ep);
            ASSERT_EQ(2, eep->_ref_count.load());
            ASSERT_EQ(eep, extended_end_point::address(tmp));
            ASSERT_EQ(eep, extended_end_point::address(ep));
        }
        ASSERT_EQ(1, eep->_ref_count.load());

        melon::end_point ep2;

        // extended endpoint assigns to original endpoint
        ep2 = ep;
        ASSERT_EQ(2, eep->_ref_count.load());
        ASSERT_EQ(eep, extended_end_point::address(ep2));

        // original endpoint assigns to extended endpoint
        ep2 = melon::end_point();
        ASSERT_EQ(1, eep->_ref_count.load());
        ASSERT_FALSE(extended_end_point::is_extended(ep2));

        // extended endpoint assigns to extended endpoint
        ASSERT_EQ(0, melon::str2endpoint("[::1]:2233", &ep2));
        extended_end_point *eep2 = extended_end_point::address(ep2);
        ASSERT_TRUE(eep2);
        ep2 = ep;
        // eep2 has been returned to resource pool, but we can still access it here unsafely.
        ASSERT_EQ(0, eep2->_ref_count.load());
        ASSERT_EQ(AF_UNSPEC, eep2->family());
        ASSERT_EQ(2, eep->_ref_count.load());
        ASSERT_EQ(eep, extended_end_point::address(ep));
        ASSERT_EQ(eep, extended_end_point::address(ep2));

        ASSERT_EQ(0, str2endpoint("[::1]:2233", &ep2));
        ASSERT_EQ(1, eep->_ref_count.load());
        eep2 = extended_end_point::address(ep2);
        ASSERT_NE(eep, eep2);
        ASSERT_EQ(1, eep2->_ref_count.load());
    }

    TEST(EndPointTest, endpoint_compare) {
        melon::end_point ep1, ep2, ep3;

        ASSERT_EQ(0, melon::str2endpoint("127.0.0.1:8080", &ep1));
        ASSERT_EQ(0, melon::str2endpoint("127.0.0.1:8080", &ep2));
        ASSERT_EQ(0, melon::str2endpoint("127.0.0.3:8080", &ep3));
        ASSERT_EQ(ep1, ep2);
        ASSERT_NE(ep1, ep3);

        ASSERT_EQ(0, melon::str2endpoint("unix:sock1.file", &ep1));
        ASSERT_EQ(0, melon::str2endpoint(" unix:sock1.file", &ep2));
        ASSERT_EQ(0, melon::str2endpoint("unix:sock3.file", &ep3));
        ASSERT_EQ(ep1, ep2);
        ASSERT_NE(ep1, ep3);

        ASSERT_EQ(0, melon::str2endpoint("[::1]:2233", &ep1));
        ASSERT_EQ(0, melon::str2endpoint("[::1]:2233", &ep2));
        ASSERT_EQ(0, melon::str2endpoint("[::3]:2233", &ep3));
        ASSERT_EQ(ep1, ep2);
        ASSERT_NE(ep1, ep3);
    }

    TEST(EndPointTest, endpoint_sockaddr_conv_ipv4) {
        melon::end_point ep;
        ASSERT_EQ(0, melon::str2endpoint("1.2.3.4:8086", &ep));

        in_addr expected_in_addr;
        bzero(&expected_in_addr, sizeof(expected_in_addr));
        expected_in_addr.s_addr = 0x04030201u;

        sockaddr_storage ss;
        sockaddr_in *in4 = (sockaddr_in *) &ss;

        memset(&ss, 'a', sizeof(ss));
        ASSERT_EQ(0, melon::endpoint2sockaddr(ep, &ss));
        ASSERT_EQ(AF_INET, ss.ss_family);
        ASSERT_EQ(AF_INET, in4->sin_family);
        in_port_t port = htons(8086);
        ASSERT_EQ(port, in4->sin_port);
        ASSERT_EQ(0, memcmp(&in4->sin_addr, &expected_in_addr, sizeof(expected_in_addr)));

        sockaddr_storage ss2;
        socklen_t ss2_size = 0;
        memset(&ss2, 'b', sizeof(ss2));
        ASSERT_EQ(0, melon::endpoint2sockaddr(ep, &ss2, &ss2_size));
        ASSERT_EQ(ss2_size, sizeof(*in4));
        ASSERT_EQ(0, memcmp(&ss2, &ss, sizeof(ss)));

        melon::end_point ep2;
        ASSERT_EQ(0, melon::sockaddr2endpoint(&ss, sizeof(*in4), &ep2));
        ASSERT_EQ(ep2, ep);

        ASSERT_EQ(AF_INET, melon::get_endpoint_type(ep));
    }

    TEST(EndPointTest, endpoint_sockaddr_conv_ipv6) {
        melon::end_point ep;
        ASSERT_EQ(0, melon::str2endpoint("[::1]:8086", &ep));

        in6_addr expect_in6_addr;
        bzero(&expect_in6_addr, sizeof(expect_in6_addr));
#if defined(MELON_PLATFORM_OSX)
        expect_in6_addr.__u6_addr.__u6_addr8[15] = 1;
#elif defined(MELON_PLATFORM_LINUX)
        expect_in6_addr.__in6_u.__u6_addr8[15] = 1;
#endif

        sockaddr_storage ss;
        const sockaddr_in6 *sa6 = (sockaddr_in6 *) &ss;

        memset(&ss, 'a', sizeof(ss));
        ASSERT_EQ(0, melon::endpoint2sockaddr(ep, &ss));
        ASSERT_EQ(AF_INET6, ss.ss_family);
        ASSERT_EQ(AF_INET6, sa6->sin6_family);
        in_port_t port = htons(8086);
        ASSERT_EQ(port, sa6->sin6_port);
        ASSERT_EQ(0u, sa6->sin6_flowinfo);
        ASSERT_EQ(0, memcmp(&expect_in6_addr, &sa6->sin6_addr, sizeof(in6_addr)));
        ASSERT_EQ(0u, sa6->sin6_scope_id);

        sockaddr_storage ss2;
        socklen_t ss2_size = 0;
        memset(&ss2, 'b', sizeof(ss2));
        ASSERT_EQ(0, melon::endpoint2sockaddr(ep, &ss2, &ss2_size));
        ASSERT_EQ(ss2_size, sizeof(*sa6));
        ASSERT_EQ(0, memcmp(&ss2, &ss, sizeof(ss)));

        melon::end_point ep2;
        ASSERT_EQ(0, melon::sockaddr2endpoint(&ss, sizeof(*sa6), &ep2));
        ASSERT_STREQ("[::1]:8086", melon::endpoint2str(ep2).c_str());

        ASSERT_EQ(AF_INET6, melon::get_endpoint_type(ep));
    }


    TEST(EndPointTest, endpoint_sockaddr_conv_unix) {
        melon::end_point ep;
        ASSERT_EQ(0, melon::str2endpoint("unix:sock.file", &ep));

        sockaddr_storage ss;
        const sockaddr_un *un = (sockaddr_un *) &ss;

        memset(&ss, 'a', sizeof(ss));
        ASSERT_EQ(0, melon::endpoint2sockaddr(ep, &ss));
        ASSERT_EQ(AF_UNIX, ss.ss_family);
        ASSERT_EQ(AF_UNIX, un->sun_family);
        ASSERT_EQ(0, memcmp("sock.file", un->sun_path, 10));

        sockaddr_storage ss2;
        socklen_t ss2_size = 0;
        memset(&ss2, 'b', sizeof(ss2));
        ASSERT_EQ(0, melon::endpoint2sockaddr(ep, &ss2, &ss2_size));
        ASSERT_EQ(offsetof(struct sockaddr_un, sun_path) + strlen("sock.file") + 1, ss2_size);
        ASSERT_EQ(0, memcmp(&ss2, &ss, sizeof(ss)));

        melon::end_point ep2;
        ASSERT_EQ(0, melon::sockaddr2endpoint(&ss, sizeof(sa_family_t) + strlen(un->sun_path) + 1, &ep2));
        ASSERT_STREQ("unix:sock.file", melon::endpoint2str(ep2).c_str());

        ASSERT_EQ(AF_UNIX, melon::get_endpoint_type(ep));
    }

    void concurrent_proc(void *p) {
        for (int i = 0; i < 10000; ++i) {
            melon::end_point ep;
            std::string str("127.0.0.1:8080");
            ASSERT_EQ(0, melon::str2endpoint(str.c_str(), &ep));
            ASSERT_EQ(str, melon::endpoint2str(ep).c_str());

            str.assign("[::1]:8080");
            ASSERT_EQ(0, melon::str2endpoint(str.c_str(), &ep));
            ASSERT_EQ(str, melon::endpoint2str(ep).c_str());

            str.assign("unix:test.sock");
            ASSERT_EQ(0, melon::str2endpoint(str.c_str(), &ep));
            ASSERT_EQ(str, melon::endpoint2str(ep).c_str());
        }
        *(int *) p = 1;
    }

    TEST(EndPointTest, endpoint_concurrency) {
        const int T = 5;
        pthread_t tids[T];
        int rets[T] = {0};
        for (int i = 0; i < T; ++i) {
            pthread_create(&tids[i], nullptr, [](void *p) {
                concurrent_proc(p);
                return (void *) nullptr;
            }, &rets[i]);
        }
        for (int i = 0; i < T; ++i) {
            pthread_join(tids[i], nullptr);
            ASSERT_EQ(1, rets[i]);
        }
    }


} // end of namespace
