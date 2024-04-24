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


// Date 2014/10/13 19:47:59

#include <pthread.h>                                // pthread_*

#include <cstddef>
#include <memory>
#include <iostream>
#include "melon/utility/time.h"
#include "melon/utility/macros.h"
#include "melon/var/recorder.h"
#include "melon/var/latency_recorder.h"
#include <gtest/gtest.h>

namespace {
TEST(RecorderTest, test_complement) {
    MLOG(INFO) << "sizeof(LatencyRecorder)=" << sizeof(melon::var::LatencyRecorder)
              << " " << sizeof(melon::var::detail::Percentile)
              << " " << sizeof(melon::var::Maxer<int64_t>)
              << " " << sizeof(melon::var::IntRecorder)
              << " " << sizeof(melon::var::Window<melon::var::IntRecorder>)
              << " " << sizeof(melon::var::Window<melon::var::detail::Percentile>);

    for (int a = -10000000; a < 10000000; ++a) {
        const uint64_t complement = melon::var::IntRecorder::_get_complement(a);
        const int64_t b = melon::var::IntRecorder::_extend_sign_bit(complement);
        ASSERT_EQ(a, b);
    }
}

TEST(RecorderTest, test_compress) {
    const uint64_t num = 125345;
    const uint64_t sum = 26032906;
    const uint64_t compressed = melon::var::IntRecorder::_compress(num, sum);
    ASSERT_EQ(num, melon::var::IntRecorder::_get_num(compressed));
    ASSERT_EQ(sum, melon::var::IntRecorder::_get_sum(compressed));
}

TEST(RecorderTest, test_compress_negtive_number) {
    for (int a = -10000000; a < 10000000; ++a) {
        const uint64_t sum = melon::var::IntRecorder::_get_complement(a);
        const uint64_t num = 123456;
        const uint64_t compressed = melon::var::IntRecorder::_compress(num, sum);
        ASSERT_EQ(num, melon::var::IntRecorder::_get_num(compressed));
        ASSERT_EQ(a, melon::var::IntRecorder::_extend_sign_bit(melon::var::IntRecorder::_get_sum(compressed)));
    }
}

TEST(RecorderTest, sanity) {
    {
        melon::var::IntRecorder recorder;
        ASSERT_TRUE(recorder.valid());
        ASSERT_EQ(0, recorder.expose("var1"));
        for (size_t i = 0; i < 100; ++i) {
            recorder << 2;
        }
        ASSERT_EQ(2l, (int64_t)recorder.average());
        ASSERT_EQ("2", melon::var::Variable::describe_exposed("var1"));
        std::vector<std::string> vars;
        melon::var::Variable::list_exposed(&vars);
        ASSERT_EQ(1UL, vars.size());
        ASSERT_EQ("var1", vars[0]);
        ASSERT_EQ(1UL, melon::var::Variable::count_exposed());
    }
    ASSERT_EQ(0UL, melon::var::Variable::count_exposed());
}

TEST(RecorderTest, window) {
    melon::var::IntRecorder c1;
    ASSERT_TRUE(c1.valid());
    melon::var::Window<melon::var::IntRecorder> w1(&c1, 1);
    melon::var::Window<melon::var::IntRecorder> w2(&c1, 2);
    melon::var::Window<melon::var::IntRecorder> w3(&c1, 3);

    const int N = 10000;
    int64_t last_time = mutil::gettimeofday_us();
    for (int i = 1; i <= N; ++i) {
        c1 << i;
        int64_t now = mutil::gettimeofday_us();
        if (now - last_time >= 1000000L) {
            last_time = now;
            MLOG(INFO) << "c1=" << c1 << " w1=" << w1 << " w2=" << w2 << " w3=" << w3;
        } else {
            usleep(950);
        }
    }
}

TEST(RecorderTest, negative) {
    melon::var::IntRecorder recorder;
    ASSERT_TRUE(recorder.valid());
    for (size_t i = 0; i < 3; ++i) {
        recorder << -2;
    }
    ASSERT_EQ(-2, recorder.average());
}

TEST(RecorderTest, positive_overflow) {
    melon::var::IntRecorder recorder1;
    ASSERT_TRUE(recorder1.valid());
    for (int i = 0; i < 5; ++i) {
        recorder1 << std::numeric_limits<int64_t>::max();
    }
    ASSERT_EQ(std::numeric_limits<int>::max(), recorder1.average());

    melon::var::IntRecorder recorder2;
    ASSERT_TRUE(recorder2.valid());
    recorder2.set_debug_name("recorder2");
    for (int i = 0; i < 5; ++i) {
        recorder2 << std::numeric_limits<int64_t>::max();
    }
    ASSERT_EQ(std::numeric_limits<int>::max(), recorder2.average());

    melon::var::IntRecorder recorder3;
    ASSERT_TRUE(recorder3.valid());
    recorder3.expose("recorder3");
    for (int i = 0; i < 5; ++i) {
        recorder3 << std::numeric_limits<int64_t>::max();
    }
    ASSERT_EQ(std::numeric_limits<int>::max(), recorder3.average());

    melon::var::LatencyRecorder latency1;
    latency1.expose("latency1");
    latency1 << std::numeric_limits<int64_t>::max();

    melon::var::LatencyRecorder latency2;
    latency2 << std::numeric_limits<int64_t>::max();
}

TEST(RecorderTest, negtive_overflow) {
    melon::var::IntRecorder recorder1;
    ASSERT_TRUE(recorder1.valid());
    for (int i = 0; i < 5; ++i) {
        recorder1 << std::numeric_limits<int64_t>::min();
    }
    ASSERT_EQ(std::numeric_limits<int>::min(), recorder1.average());

    melon::var::IntRecorder recorder2;
    ASSERT_TRUE(recorder2.valid());
    recorder2.set_debug_name("recorder2");
    for (int i = 0; i < 5; ++i) {
        recorder2 << std::numeric_limits<int64_t>::min();
    }
    ASSERT_EQ(std::numeric_limits<int>::min(), recorder2.average());

    melon::var::IntRecorder recorder3;
    ASSERT_TRUE(recorder3.valid());
    recorder3.expose("recorder3");
    for (int i = 0; i < 5; ++i) {
        recorder3 << std::numeric_limits<int64_t>::min();
    }
    ASSERT_EQ(std::numeric_limits<int>::min(), recorder3.average());

    melon::var::LatencyRecorder latency1;
    latency1.expose("latency1");
    latency1 << std::numeric_limits<int64_t>::min();

    melon::var::LatencyRecorder latency2;
    latency2 << std::numeric_limits<int64_t>::min();
}

const size_t OPS_PER_THREAD = 20000000;

static void *thread_counter(void *arg) {
    melon::var::IntRecorder *recorder = (melon::var::IntRecorder *)arg;
    mutil::Timer timer;
    timer.start();
    for (int i = 0; i < (int)OPS_PER_THREAD; ++i) {
        *recorder << i;
    }
    timer.stop();
    return (void *)(timer.n_elapsed());
}

TEST(RecorderTest, perf) {
    melon::var::IntRecorder recorder;
    ASSERT_TRUE(recorder.valid());
    pthread_t threads[8];
    for (size_t i = 0; i < ARRAY_SIZE(threads); ++i) {
        pthread_create(&threads[i], NULL, &thread_counter, (void *)&recorder);
    }
    long totol_time = 0;
    for (size_t i = 0; i < ARRAY_SIZE(threads); ++i) {
        void *ret; 
        pthread_join(threads[i], &ret);
        totol_time += (long)ret;
    }
    ASSERT_EQ(((int64_t)OPS_PER_THREAD - 1) / 2, recorder.average());
    MLOG(INFO) << "Recorder takes " << totol_time / (OPS_PER_THREAD * ARRAY_SIZE(threads))
              << "ns per sample with " << ARRAY_SIZE(threads) 
              << " threads";
}

TEST(RecorderTest, latency_recorder_qps_accuracy) {
    melon::var::LatencyRecorder lr1(2); // set windows size to 2s
    melon::var::LatencyRecorder lr2(2);
    melon::var::LatencyRecorder lr3(2);
    melon::var::LatencyRecorder lr4(2);
    usleep(3000000); // wait sampler to sample 3 times

    auto write = [](melon::var::LatencyRecorder& lr, int times) {
        for (int i = 0; i < times; ++i) {
            lr << 1;
        }
    };
    write(lr1, 10);
    write(lr2, 11);
    write(lr3, 3);
    write(lr4, 1);
    usleep(1000000); // wait sampler to sample 1 time

    auto read = [](melon::var::LatencyRecorder& lr, double exp_qps, int window_size = 0) {
        int64_t qps_sum = 0;
        int64_t exp_qps_int = (int64_t)exp_qps;
        for (int i = 0; i < 1000; ++i) {
            int64_t qps = window_size ? lr.qps(window_size): lr.qps();
            EXPECT_GE(qps, exp_qps_int - 1);
            EXPECT_LE(qps, exp_qps_int + 1);
            qps_sum += qps;
        }
        double err = fabs(qps_sum / 1000.0 - exp_qps);
        return err;
    };
    ASSERT_GT(0.1, read(lr1, 10/2.0));
    ASSERT_GT(0.1, read(lr2, 11/2.0));
    ASSERT_GT(0.1, read(lr3, 3/2.0));
    ASSERT_GT(0.1, read(lr4, 1/2.0));

    ASSERT_GT(0.1, read(lr1, 10/3.0, 3));
    ASSERT_GT(0.2, read(lr2, 11/3.0, 3));
    ASSERT_GT(0.1, read(lr3, 3/3.0, 3));
    ASSERT_GT(0.1, read(lr4, 1/3.0, 3));
}

} // namespace
