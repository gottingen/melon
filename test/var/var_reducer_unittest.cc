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


#include <limits>                           //std::numeric_limits

#include <melon/var/reducer.h>

#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <melon/utility/string_printf.h>
#include <melon/utility/string_splitter.h>
#include <melon/utility/config.h>
#include <gtest/gtest.h>

namespace {
class ReducerTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(ReducerTest, atomicity) {
    ASSERT_EQ(sizeof(int32_t), sizeof(melon::var::detail::ElementContainer<int32_t>));
    ASSERT_EQ(sizeof(int64_t), sizeof(melon::var::detail::ElementContainer<int64_t>));
    ASSERT_EQ(sizeof(float), sizeof(melon::var::detail::ElementContainer<float>));
    ASSERT_EQ(sizeof(double), sizeof(melon::var::detail::ElementContainer<double>));
}

TEST_F(ReducerTest, adder) {    
    melon::var::Adder<uint32_t> reducer1;
    ASSERT_TRUE(reducer1.valid());
    reducer1 << 2 << 4;
    ASSERT_EQ(6ul, reducer1.get_value());

    melon::var::Adder<double> reducer2;
    ASSERT_TRUE(reducer2.valid());
    reducer2 << 2.0 << 4.0;
    ASSERT_DOUBLE_EQ(6.0, reducer2.get_value());

    melon::var::Adder<int> reducer3;
    ASSERT_TRUE(reducer3.valid());
    reducer3 << -9 << 1 << 0 << 3;
    ASSERT_EQ(-5, reducer3.get_value());
}

const size_t OPS_PER_THREAD = 500000;

static void *thread_counter(void *arg) {
    melon::var::Adder<uint64_t> *reducer = (melon::var::Adder<uint64_t> *)arg;
    mutil::Timer timer;
    timer.start();
    for (size_t i = 0; i < OPS_PER_THREAD; ++i) {
        (*reducer) << 2;
    }
    timer.stop();
    return (void *)(timer.n_elapsed());
}

void *add_atomic(void *arg) {
    mutil::atomic<uint64_t> *counter = (mutil::atomic<uint64_t> *)arg;
    mutil::Timer timer;
    timer.start();
    for (size_t i = 0; i < OPS_PER_THREAD / 100; ++i) {
        counter->fetch_add(2, mutil::memory_order_relaxed);
    }
    timer.stop();
    return (void *)(timer.n_elapsed());
}

static long start_perf_test_with_atomic(size_t num_thread) {
    mutil::atomic<uint64_t> counter(0);
    pthread_t threads[num_thread];
    for (size_t i = 0; i < num_thread; ++i) {
        pthread_create(&threads[i], NULL, &add_atomic, (void *)&counter);
    }
    long totol_time = 0;
    for (size_t i = 0; i < num_thread; ++i) {
        void *ret; 
        pthread_join(threads[i], &ret);
        totol_time += (long)ret;
    }
    long avg_time = totol_time / (OPS_PER_THREAD / 100 * num_thread);
    EXPECT_EQ(2ul * num_thread * OPS_PER_THREAD / 100, counter.load());
    return avg_time;
}

static long start_perf_test_with_adder(size_t num_thread) {
    melon::var::Adder<uint64_t> reducer;
    EXPECT_TRUE(reducer.valid());
    pthread_t threads[num_thread];
    for (size_t i = 0; i < num_thread; ++i) {
        pthread_create(&threads[i], NULL, &thread_counter, (void *)&reducer);
    }
    long totol_time = 0;
    for (size_t i = 0; i < num_thread; ++i) {
        void *ret = NULL; 
        pthread_join(threads[i], &ret);
        totol_time += (long)ret;
    }
    long avg_time = totol_time / (OPS_PER_THREAD * num_thread);
    EXPECT_EQ(2ul * num_thread * OPS_PER_THREAD, reducer.get_value());
    return avg_time;
}

TEST_F(ReducerTest, perf) {
    std::ostringstream oss;
    for (size_t i = 1; i <= 24; ++i) {
        oss << i << '\t' << start_perf_test_with_adder(i) << '\n';
    }
    LOG(INFO) << "Adder performance:\n" << oss.str();
    oss.str("");
    for (size_t i = 1; i <= 24; ++i) {
        oss << i << '\t' << start_perf_test_with_atomic(i) << '\n';
    }
    LOG(INFO) << "Atomic performance:\n" << oss.str();
}

TEST_F(ReducerTest, Min) {
    melon::var::Miner<uint64_t> reducer;
    ASSERT_EQ(std::numeric_limits<uint64_t>::max(), reducer.get_value());
    reducer << 10 << 20;
    ASSERT_EQ(10ul, reducer.get_value());
    reducer << 5;
    ASSERT_EQ(5ul, reducer.get_value());
    reducer << std::numeric_limits<uint64_t>::max();
    ASSERT_EQ(5ul, reducer.get_value());
    reducer << 0;
    ASSERT_EQ(0ul, reducer.get_value());

    melon::var::Miner<int> reducer2;
    ASSERT_EQ(std::numeric_limits<int>::max(), reducer2.get_value());
    reducer2 << 10 << 20;
    ASSERT_EQ(10, reducer2.get_value());
    reducer2 << -5;
    ASSERT_EQ(-5, reducer2.get_value());
    reducer2 << std::numeric_limits<int>::max();
    ASSERT_EQ(-5, reducer2.get_value());
    reducer2 << 0;
    ASSERT_EQ(-5, reducer2.get_value());
    reducer2 << std::numeric_limits<int>::min();
    ASSERT_EQ(std::numeric_limits<int>::min(), reducer2.get_value());
}

TEST_F(ReducerTest, max) {
    melon::var::Maxer<uint64_t> reducer;
    ASSERT_EQ(std::numeric_limits<uint64_t>::min(), reducer.get_value());
    ASSERT_TRUE(reducer.valid());
    reducer << 20 << 10;
    ASSERT_EQ(20ul, reducer.get_value());
    reducer << 30;
    ASSERT_EQ(30ul, reducer.get_value());
    reducer << 0;
    ASSERT_EQ(30ul, reducer.get_value());

    melon::var::Maxer<int> reducer2;
    ASSERT_EQ(std::numeric_limits<int>::min(), reducer2.get_value());
    ASSERT_TRUE(reducer2.valid());
    reducer2 << 20 << 10;
    ASSERT_EQ(20, reducer2.get_value());
    reducer2 << 30;
    ASSERT_EQ(30, reducer2.get_value());
    reducer2 << 0;
    ASSERT_EQ(30, reducer2.get_value());
    reducer2 << std::numeric_limits<int>::max();
    ASSERT_EQ(std::numeric_limits<int>::max(), reducer2.get_value());
}

melon::var::Adder<long> g_a;

TEST_F(ReducerTest, global) {
    ASSERT_TRUE(g_a.valid());
    g_a.get_value();
}

void ReducerTest_window() {
    melon::var::Adder<int> c1;
    melon::var::Maxer<int> c2;
    melon::var::Miner<int> c3;
    melon::var::Window<melon::var::Adder<int> > w1(&c1, 1);
    melon::var::Window<melon::var::Adder<int> > w2(&c1, 2);
    melon::var::Window<melon::var::Adder<int> > w3(&c1, 3);
    melon::var::Window<melon::var::Maxer<int> > w4(&c2, 1);
    melon::var::Window<melon::var::Maxer<int> > w5(&c2, 2);
    melon::var::Window<melon::var::Maxer<int> > w6(&c2, 3);
    melon::var::Window<melon::var::Miner<int> > w7(&c3, 1);
    melon::var::Window<melon::var::Miner<int> > w8(&c3, 2);
    melon::var::Window<melon::var::Miner<int> > w9(&c3, 3);

#if !MELON_WITH_GLOG
    logging::StringSink log_str;
    logging::LogSink* old_sink = logging::SetLogSink(&log_str);
    c2.get_value();
    ASSERT_EQ(&log_str, logging::SetLogSink(old_sink));
    ASSERT_NE(std::string::npos, log_str.find(
                  "You should not call Reducer<int, melon::var::detail::MaxTo<int>>"
                  "::get_value() when a Window<> is used because the operator"
                  " does not have inverse."));
#endif
    const int N = 6000;
    int count = 0;
    int total_count = 0;
    int64_t last_time = mutil::gettimeofday_us();
    for (int i = 1; i <= N; ++i) {
        c1 << 1;
        c2 << N - i;
        c3 << i;
        ++count;
        ++total_count;
        int64_t now = mutil::gettimeofday_us();
        if (now - last_time >= 1000000L) {
            last_time = now;
            ASSERT_EQ(total_count, c1.get_value());
            LOG(INFO) << "c1=" << total_count
                      << " count=" << count
                      << " w1=" << w1
                      << " w2=" << w2
                      << " w3=" << w3
                      << " w4=" << w4
                      << " w5=" << w5
                      << " w6=" << w6
                      << " w7=" << w7
                      << " w8=" << w8
                      << " w9=" << w9;
            count = 0;
        } else {
            usleep(950);
        }
    }
}

TEST_F(ReducerTest, window) {
#if !MELON_WITH_GLOG
    ReducerTest_window();
    logging::StringSink log_str;
    logging::LogSink* old_sink = logging::SetLogSink(&log_str);
    sleep(1);
    ASSERT_EQ(&log_str, logging::SetLogSink(old_sink));
    if (log_str.find("Removed ") != std::string::npos) {
        ASSERT_NE(std::string::npos, log_str.find("Removed 3, sampled 0")) << log_str;
    }
#endif
}

struct Foo {
    int x;
    Foo() : x(0) {}
    explicit Foo(int x2) : x(x2) {}
    void operator+=(const Foo& rhs) {
        x += rhs.x;
    }
};

std::ostream& operator<<(std::ostream& os, const Foo& f) {
    return os << "Foo{" << f.x << "}";
}

TEST_F(ReducerTest, non_primitive) {
    melon::var::Adder<Foo> adder;
    adder << Foo(2) << Foo(3) << Foo(4);
    ASSERT_EQ(9, adder.get_value().x);
}

bool g_stop = false;
struct StringAppenderResult {
    int count;
};

static void* string_appender(void* arg) {
    melon::var::Adder<std::string>* cater = (melon::var::Adder<std::string>*)arg;
    int count = 0;
    std::string id = mutil::string_printf("%lld", (long long)pthread_self());
    std::string tmp = "a";
    for (count = 0; !count || !g_stop; ++count) {
        *cater << id << ":";
        for (char c = 'a'; c <= 'z'; ++c) {
            tmp[0] = c;
            *cater << tmp;
        }
        *cater << ".";
    }
    StringAppenderResult* res = new StringAppenderResult;
    res->count = count;
    LOG(INFO) << "Appended " << count;
    return res;
}

TEST_F(ReducerTest, non_primitive_mt) {
    melon::var::Adder<std::string> cater;
    pthread_t th[8];
    g_stop = false;
    for (size_t i = 0; i < arraysize(th); ++i) {
        pthread_create(&th[i], NULL, string_appender, &cater);
    }
    usleep(50000);
    g_stop = true;
    mutil::hash_map<pthread_t, int> appended_count;
    for (size_t i = 0; i < arraysize(th); ++i) {
        StringAppenderResult* res = NULL;
        pthread_join(th[i], (void**)&res);
        appended_count[th[i]] = res->count;
        delete res;
    }
    mutil::hash_map<pthread_t, int> got_count;
    std::string res = cater.get_value();
    for (mutil::StringSplitter sp(res.c_str(), '.'); sp; ++sp) {
        char* endptr = NULL;
        ++got_count[(pthread_t)strtoll(sp.field(), &endptr, 10)];
        ASSERT_EQ(27LL, sp.field() + sp.length() - endptr)
            << mutil::StringPiece(sp.field(), sp.length());
        ASSERT_EQ(0, memcmp(":abcdefghijklmnopqrstuvwxyz", endptr, 27));
    }
    ASSERT_EQ(appended_count.size(), got_count.size());
    for (size_t i = 0; i < arraysize(th); ++i) {
        ASSERT_EQ(appended_count[th[i]], got_count[th[i]]);
    }
}

TEST_F(ReducerTest, simple_window) {
    melon::var::Adder<int64_t> a;
    melon::var::Window<melon::var::Adder<int64_t> > w(&a, 10);
    a << 100;
    sleep(3);
    const int64_t v = w.get_value();
    ASSERT_EQ(100, v) << "v=" << v;
}
} // namespace
