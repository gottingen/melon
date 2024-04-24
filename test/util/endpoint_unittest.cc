// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#include <gtest/gtest.h>
#include "melon/utility/errno.h"
#include "melon/utility/endpoint.h"
#include "melon/utility/logging.h"
#include "melon/utility/containers/flat_map.h"
#include "melon/utility/details/extended_endpoint.hpp"

namespace {

using mutil::details::ExtendedEndPoint;

TEST(EndPointTest, comparisons) {
    mutil::EndPoint p1(mutil::int2ip(1234), 5678);
    mutil::EndPoint p2 = p1;
    ASSERT_TRUE(p1 == p2 && !(p1 != p2));
    ASSERT_TRUE(p1 <= p2 && p1 >= p2 && !(p1 < p2 || p1 > p2));
    ++p2.port;
    ASSERT_TRUE(p1 != p2 && !(p1 == p2));
    ASSERT_TRUE(p1 < p2 && p2 > p1 && !(p2 <= p1 || p1 >= p2));
    --p2.port;
    p2.ip = mutil::int2ip(mutil::ip2int(p2.ip)-1);
    ASSERT_TRUE(p1 != p2 && !(p1 == p2));
    ASSERT_TRUE(p1 > p2 && p2 < p1 && !(p1 <= p2 || p2 >= p1));
}

TEST(EndPointTest, ip_t) {
    MLOG(INFO) << "INET_ADDRSTRLEN = " << INET_ADDRSTRLEN;
    
    mutil::ip_t ip0;
    ASSERT_EQ(0, mutil::str2ip("1.1.1.1", &ip0));
    ASSERT_STREQ("1.1.1.1", mutil::ip2str(ip0).c_str());
    ASSERT_EQ(-1, mutil::str2ip("301.1.1.1", &ip0));
    ASSERT_EQ(-1, mutil::str2ip("1.-1.1.1", &ip0));
    ASSERT_EQ(-1, mutil::str2ip("1.1.-101.1", &ip0));
    ASSERT_STREQ("1.0.0.0", mutil::ip2str(mutil::int2ip(1)).c_str());

    mutil::ip_t ip1, ip2, ip3;
    ASSERT_EQ(0, mutil::str2ip("192.168.0.1", &ip1));
    ASSERT_EQ(0, mutil::str2ip("192.168.0.2", &ip2));
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
    MLOG(INFO) << "my_ip is " << mutil::my_ip() << std::endl
              << "my_ip_cstr is " << mutil::my_ip_cstr() << std::endl
              << "my_hostname is " << mutil::my_hostname();
}

TEST(EndPointTest, endpoint) {
    mutil::EndPoint p1;
    ASSERT_EQ(mutil::IP_ANY, p1.ip);
    ASSERT_EQ(0, p1.port);
    
    mutil::EndPoint p2(mutil::IP_NONE, -1);
    ASSERT_EQ(mutil::IP_NONE, p2.ip);
    ASSERT_EQ(-1, p2.port);

    mutil::EndPoint p3;
    ASSERT_EQ(-1, mutil::str2endpoint(" 127.0.0.1:-1", &p3));
    ASSERT_EQ(-1, mutil::str2endpoint(" 127.0.0.1:65536", &p3));
    ASSERT_EQ(0, mutil::str2endpoint(" 127.0.0.1:65535", &p3));
    ASSERT_EQ(0, mutil::str2endpoint(" 127.0.0.1:0", &p3));

    mutil::EndPoint p4;
    ASSERT_EQ(0, mutil::str2endpoint(" 127.0.0.1: 289 ", &p4));
    ASSERT_STREQ("127.0.0.1", mutil::ip2str(p4.ip).c_str());
    ASSERT_EQ(289, p4.port);
    
    mutil::EndPoint p5;
    ASSERT_EQ(-1, hostname2endpoint("localhost:-1", &p5));
    ASSERT_EQ(-1, hostname2endpoint("localhost:65536", &p5));
    ASSERT_EQ(0, hostname2endpoint("localhost:65535", &p5)) << berror();
    ASSERT_EQ(0, hostname2endpoint("localhost:0", &p5));

}

TEST(EndPointTest, hash_table) {
    mutil::hash_map<mutil::EndPoint, int> m;
    mutil::EndPoint ep1(mutil::IP_ANY, 123);
    mutil::EndPoint ep2(mutil::IP_ANY, 456);
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
    mutil::FlatMap<mutil::EndPoint, int> m;
    ASSERT_EQ(0, m.init(1024));
    uint32_t port = 8088;

    mutil::EndPoint ep1(mutil::IP_ANY, port);
    mutil::EndPoint ep2(mutil::IP_ANY, port);
    ++m[ep1];
    ++m[ep2];
    ASSERT_EQ(1u, m.size());

    mutil::ip_t ip_addr;
    mutil::str2ip("10.10.10.10", &ip_addr);
    int ip = mutil::ip2int(ip_addr);

    for (int i = 0; i < 1023; ++i) {
        mutil::EndPoint ep(mutil::int2ip(++ip), port);
        ++m[ep];
    }

    mutil::BucketInfo info = m.bucket_info();
    MLOG(INFO) << "bucket info max long=" << info.longest_length
        << " avg=" << info.average_length << std::endl;
    ASSERT_LT(info.longest_length, 32ul) << "detect hash collision and it's too large.";
}

void* server_proc(void* arg) {
    int listen_fd = (int64_t)arg;
    sockaddr_storage ss;
    socklen_t len = sizeof(ss);
    int fd = accept(listen_fd, (sockaddr*)&ss, &len);
    return (void*)(int64_t)fd;
}

static void test_listen_connect(const std::string& server_addr, const std::string& exp_client_addr) {
    mutil::EndPoint point;
    ASSERT_EQ(0, mutil::str2endpoint(server_addr.c_str(), &point));
    ASSERT_EQ(server_addr, mutil::endpoint2str(point).c_str());

    int listen_fd = mutil::tcp_listen(point);
    ASSERT_GT(listen_fd, 0);
    pthread_t pid;
    pthread_create(&pid, NULL, server_proc, (void*)(int64_t)listen_fd);

    int fd = mutil::tcp_connect(point, NULL);
    ASSERT_GT(fd, 0);

    mutil::EndPoint point2;
    ASSERT_EQ(0, mutil::get_local_side(fd, &point2));

    std::string s = mutil::endpoint2str(point2).c_str();
    if (mutil::get_endpoint_type(point2) == AF_UNIX) {
        ASSERT_EQ(exp_client_addr, s);
    } else {
        ASSERT_EQ(exp_client_addr, s.substr(0, exp_client_addr.size()));
    }
    ASSERT_EQ(0, mutil::get_remote_side(fd, &point2));
    ASSERT_EQ(server_addr, mutil::endpoint2str(point2).c_str());
    close(fd);

    void* ret = nullptr;
    pthread_join(pid, &ret);
    int server_fd = (int)(int64_t)ret;
    ASSERT_GT(server_fd, 0);
    close(server_fd);
    close(listen_fd);
}

static void test_parse_and_serialize(const std::string& instr, const std::string& outstr) {
    mutil::EndPoint ep;
    ASSERT_EQ(0, mutil::str2endpoint(instr.c_str(), &ep));
    mutil::EndPointStr s = mutil::endpoint2str(ep);
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

    mutil::EndPoint ep;
    ASSERT_EQ(-1, mutil::str2endpoint("[2001:db8:1:2:3:ab9:c0a8:102]", &ep));
    ASSERT_EQ(-1, mutil::str2endpoint("[2001:db8:1:2:3:ab9:c0a8:102]#654321", &ep));
    ASSERT_EQ(-1, mutil::str2endpoint("ipv6:2001:db8:1:2:3:ab9:c0a8:102", &ep));
    ASSERT_EQ(-1, mutil::str2endpoint("[", &ep));
    ASSERT_EQ(-1, mutil::str2endpoint("[::1", &ep));
    ASSERT_EQ(-1, mutil::str2endpoint("[]:80", &ep));
    ASSERT_EQ(-1, mutil::str2endpoint("[]", &ep));
    ASSERT_EQ(-1, mutil::str2endpoint("[]:", &ep));
}

TEST(EndPointTest, unix_socket) {
    ::unlink("test.sock");
    test_listen_connect("unix:test.sock", "unix:");
    ::unlink("test.sock");

    mutil::EndPoint point;
    ASSERT_EQ(-1, mutil::str2endpoint("", &point));
    ASSERT_EQ(-1, mutil::str2endpoint("a.sock", &point));
    ASSERT_EQ(-1, mutil::str2endpoint("unix:", &point));
    ASSERT_EQ(-1, mutil::str2endpoint(" unix: ", &point));
    ASSERT_EQ(0, mutil::str2endpoint("unix://a.sock", 123, &point));
    ASSERT_EQ(std::string("unix://a.sock"), mutil::endpoint2str(point).c_str());

    std::string long_path = "unix:";
    long_path.append(sizeof(sockaddr_un::sun_path) - 1, 'a');
    ASSERT_EQ(0, mutil::str2endpoint(long_path.c_str(), &point));
    ASSERT_EQ(long_path, mutil::endpoint2str(point).c_str());
    long_path.push_back('a');
    ASSERT_EQ(-1, mutil::str2endpoint(long_path.c_str(), &point));
    char buf[128] = {0}; // braft use this size of buffer
    size_t ret = snprintf(buf, sizeof(buf), "%s:%d", mutil::endpoint2str(point).c_str(), INT_MAX);
    ASSERT_LT(ret, sizeof(buf) - 1);
}

TEST(EndPointTest, original_endpoint) {
    mutil::EndPoint ep;
    ASSERT_FALSE(ExtendedEndPoint::is_extended(ep));
    ASSERT_EQ(NULL, ExtendedEndPoint::address(ep));

    ASSERT_EQ(0, mutil::str2endpoint("1.2.3.4:5678", &ep));
    ASSERT_FALSE(ExtendedEndPoint::is_extended(ep));
    ASSERT_EQ(NULL, ExtendedEndPoint::address(ep));

    // ctor & dtor
    {
        mutil::EndPoint ep2(ep);
        ASSERT_FALSE(ExtendedEndPoint::is_extended(ep));
        ASSERT_EQ(ep.ip, ep2.ip);
        ASSERT_EQ(ep.port, ep2.port);
    }

    // assign
    mutil::EndPoint ep2;
    ep2 = ep;
    ASSERT_EQ(ep.ip, ep2.ip);
    ASSERT_EQ(ep.port, ep2.port);
}

TEST(EndPointTest, extended_endpoint) {
    mutil::EndPoint ep;
    ASSERT_EQ(0, mutil::str2endpoint("unix:sock.file", &ep));
    ASSERT_TRUE(ExtendedEndPoint::is_extended(ep));
    ExtendedEndPoint* eep = ExtendedEndPoint::address(ep);
    ASSERT_TRUE(eep);
    ASSERT_EQ(AF_UNIX, eep->family());
    ASSERT_EQ(1, eep->_ref_count.load());

    // copy ctor & dtor
    {
        mutil::EndPoint tmp(ep);
        ASSERT_EQ(2, eep->_ref_count.load());
        ASSERT_EQ(eep, ExtendedEndPoint::address(tmp));
        ASSERT_EQ(eep, ExtendedEndPoint::address(ep));
    }
    ASSERT_EQ(1, eep->_ref_count.load());

    mutil::EndPoint ep2;

    // extended endpoint assigns to original endpoint
    ep2 = ep;
    ASSERT_EQ(2, eep->_ref_count.load());
    ASSERT_EQ(eep, ExtendedEndPoint::address(ep2));

    // original endpoint assigns to extended endpoint
    ep2 = mutil::EndPoint();
    ASSERT_EQ(1, eep->_ref_count.load());
    ASSERT_FALSE(ExtendedEndPoint::is_extended(ep2));

    // extended endpoint assigns to extended endpoint
    ASSERT_EQ(0, mutil::str2endpoint("[::1]:2233", &ep2));
    ExtendedEndPoint* eep2 = ExtendedEndPoint::address(ep2);
    ASSERT_TRUE(eep2);
    ep2 = ep;
    // eep2 has been returned to resource pool, but we can still access it here unsafely.
    ASSERT_EQ(0, eep2->_ref_count.load());
    ASSERT_EQ(AF_UNSPEC, eep2->family());
    ASSERT_EQ(2, eep->_ref_count.load());
    ASSERT_EQ(eep, ExtendedEndPoint::address(ep));
    ASSERT_EQ(eep, ExtendedEndPoint::address(ep2));

    ASSERT_EQ(0, str2endpoint("[::1]:2233", &ep2));
    ASSERT_EQ(1, eep->_ref_count.load());
    eep2 = ExtendedEndPoint::address(ep2);
    ASSERT_NE(eep, eep2);
    ASSERT_EQ(1, eep2->_ref_count.load());
}

TEST(EndPointTest, endpoint_compare) {
    mutil::EndPoint ep1, ep2, ep3;

    ASSERT_EQ(0, mutil::str2endpoint("127.0.0.1:8080", &ep1));
    ASSERT_EQ(0, mutil::str2endpoint("127.0.0.1:8080", &ep2));
    ASSERT_EQ(0, mutil::str2endpoint("127.0.0.3:8080", &ep3));
    ASSERT_EQ(ep1, ep2);
    ASSERT_NE(ep1, ep3);

    ASSERT_EQ(0, mutil::str2endpoint("unix:sock1.file", &ep1));
    ASSERT_EQ(0, mutil::str2endpoint("unix:sock1.file", &ep2));
    ASSERT_EQ(0, mutil::str2endpoint("unix:sock3.file", &ep3));
    ASSERT_EQ(ep1, ep2);
    ASSERT_NE(ep1, ep3);

    ASSERT_EQ(0, mutil::str2endpoint("[::1]:2233", &ep1));
    ASSERT_EQ(0, mutil::str2endpoint("[::1]:2233", &ep2));
    ASSERT_EQ(0, mutil::str2endpoint("[::3]:2233", &ep3));
    ASSERT_EQ(ep1, ep2);
    ASSERT_NE(ep1, ep3);
}

TEST(EndPointTest, endpoint_sockaddr_conv_ipv4) {
    mutil::EndPoint ep;
    ASSERT_EQ(0, mutil::str2endpoint("1.2.3.4:8086", &ep));

    in_addr expected_in_addr;
    bzero(&expected_in_addr, sizeof(expected_in_addr));
    expected_in_addr.s_addr = 0x04030201u;

    sockaddr_storage ss;
    sockaddr_in* in4 = (sockaddr_in*) &ss;

    memset(&ss, 'a', sizeof(ss));
    ASSERT_EQ(0, mutil::endpoint2sockaddr(ep, &ss));
    ASSERT_EQ(AF_INET, ss.ss_family);
    ASSERT_EQ(AF_INET, in4->sin_family);
    in_port_t port = htons(8086);
    ASSERT_EQ(port, in4->sin_port);
    ASSERT_EQ(0, memcmp(&in4->sin_addr, &expected_in_addr, sizeof(expected_in_addr)));

    sockaddr_storage ss2;
    socklen_t ss2_size = 0;
    memset(&ss2, 'b', sizeof(ss2));
    ASSERT_EQ(0, mutil::endpoint2sockaddr(ep, &ss2, &ss2_size));
    ASSERT_EQ(ss2_size, sizeof(*in4));
    ASSERT_EQ(0, memcmp(&ss2, &ss, sizeof(ss)));

    mutil::EndPoint ep2;
    ASSERT_EQ(0, mutil::sockaddr2endpoint(&ss, sizeof(*in4), &ep2));
    ASSERT_EQ(ep2, ep);

    ASSERT_EQ(AF_INET, mutil::get_endpoint_type(ep));
}

TEST(EndPointTest, endpoint_sockaddr_conv_ipv6) {
    mutil::EndPoint ep;
    ASSERT_EQ(0, mutil::str2endpoint("[::1]:8086", &ep));

    in6_addr expect_in6_addr;
    bzero(&expect_in6_addr, sizeof(expect_in6_addr));
    expect_in6_addr.s6_addr[15] = 1;

    sockaddr_storage ss;
    const sockaddr_in6* sa6 = (sockaddr_in6*) &ss;

    memset(&ss, 'a', sizeof(ss));
    ASSERT_EQ(0, mutil::endpoint2sockaddr(ep, &ss));
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
    ASSERT_EQ(0, mutil::endpoint2sockaddr(ep, &ss2, &ss2_size));
    ASSERT_EQ(ss2_size, sizeof(*sa6));
    ASSERT_EQ(0, memcmp(&ss2, &ss, sizeof(ss)));

    mutil::EndPoint ep2;
    ASSERT_EQ(0, mutil::sockaddr2endpoint(&ss, sizeof(*sa6), &ep2));
    ASSERT_STREQ("[::1]:8086", mutil::endpoint2str(ep2).c_str());

    ASSERT_EQ(AF_INET6, mutil::get_endpoint_type(ep));
}

TEST(EndPointTest, endpoint_sockaddr_conv_unix) {
    mutil::EndPoint ep;
    ASSERT_EQ(0, mutil::str2endpoint("unix:sock.file", &ep));

    sockaddr_storage ss;
    const sockaddr_un* un = (sockaddr_un*) &ss;

    memset(&ss, 'a', sizeof(ss));
    ASSERT_EQ(0, mutil::endpoint2sockaddr(ep, &ss));
    ASSERT_EQ(AF_UNIX, ss.ss_family);
    ASSERT_EQ(AF_UNIX, un->sun_family);
    ASSERT_EQ(0, memcmp("sock.file", un->sun_path, 10));

    sockaddr_storage ss2;
    socklen_t ss2_size = 0;
    memset(&ss2, 'b', sizeof(ss2));
    ASSERT_EQ(0, mutil::endpoint2sockaddr(ep, &ss2, &ss2_size));
    ASSERT_EQ(offsetof(struct sockaddr_un, sun_path) + strlen("sock.file") + 1, ss2_size);
    ASSERT_EQ(0, memcmp(&ss2, &ss, sizeof(ss)));

    mutil::EndPoint ep2;
    ASSERT_EQ(0, mutil::sockaddr2endpoint(&ss, sizeof(sa_family_t) + strlen(un->sun_path) + 1, &ep2));
    ASSERT_STREQ("unix:sock.file", mutil::endpoint2str(ep2).c_str());

    ASSERT_EQ(AF_UNIX, mutil::get_endpoint_type(ep));
}

void concurrent_proc(void* p) {
    for (int i = 0; i < 10000; ++i) {
        mutil::EndPoint ep;
        std::string str("127.0.0.1:8080");
        ASSERT_EQ(0, mutil::str2endpoint(str.c_str(), &ep));
        ASSERT_EQ(str, mutil::endpoint2str(ep).c_str());

        str.assign("[::1]:8080");
        ASSERT_EQ(0, mutil::str2endpoint(str.c_str(), &ep));
        ASSERT_EQ(str, mutil::endpoint2str(ep).c_str());

        str.assign("unix:test.sock");
        ASSERT_EQ(0, mutil::str2endpoint(str.c_str(), &ep));
        ASSERT_EQ(str, mutil::endpoint2str(ep).c_str());
    }
    *(int*)p = 1;
}

TEST(EndPointTest, endpoint_concurrency) {
    const int T = 5;
    pthread_t tids[T];
    int rets[T] = {0};
    for (int i = 0; i < T; ++i) {
        pthread_create(&tids[i], nullptr, [](void* p) {
            concurrent_proc(p);
            return (void*)nullptr;
        }, &rets[i]);
    }
    for (int i = 0; i < T; ++i) {
        pthread_join(tids[i], nullptr);
        ASSERT_EQ(1, rets[i]);
    }
}

} // end of namespace
