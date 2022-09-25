
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/gtest_wrap.h"
#include <pthread.h>                                // pthread_*

#include <cstddef>
#include <memory>
#include <iostream>
#include "melon/times/time.h"
#include "melon/metrics/recorder.h"
#include "melon/metrics/latency_recorder.h"
#include "melon/metrics/gauge.h"
#include "melon/strings/str_join.h"

namespace {
    TEST(RecorderTest, test_complement) {
        MELON_LOG(INFO) << "sizeof(LatencyRecorder)=" << sizeof(melon::LatencyRecorder)
                        << " " << sizeof(melon::metrics_detail::percentile)
                        << " " << sizeof(melon::max_gauge<int64_t>)
                        << " " << sizeof(melon::IntRecorder)
                        << " " << sizeof(melon::window<melon::IntRecorder>)
                        << " " << sizeof(melon::window<melon::metrics_detail::percentile>);

        for (int a = -10000000; a < 10000000; ++a) {
            const uint64_t complement = melon::IntRecorder::_get_complement(a);
            const int64_t b = melon::IntRecorder::_extend_sign_bit(complement);
            ASSERT_EQ(a, b);
        }
    }

    TEST(RecorderTest, test_compress) {
        const uint64_t num = 125345;
        const uint64_t sum = 26032906;
        const uint64_t compressed = melon::IntRecorder::_compress(num, sum);
        ASSERT_EQ(num, melon::IntRecorder::_get_num(compressed));
        ASSERT_EQ(sum, melon::IntRecorder::_get_sum(compressed));
    }

    TEST(RecorderTest, test_compress_negtive_number) {
        for (int a = -10000000; a < 10000000; ++a) {
            const uint64_t sum = melon::IntRecorder::_get_complement(a);
            const uint64_t num = 123456;
            const uint64_t compressed = melon::IntRecorder::_compress(num, sum);
            ASSERT_EQ(num, melon::IntRecorder::_get_num(compressed));
            ASSERT_EQ(a, melon::IntRecorder::_extend_sign_bit(melon::IntRecorder::_get_sum(compressed)));
        }
    }

    TEST(RecorderTest, sanity) {
        {
            melon::IntRecorder recorder;
            ASSERT_TRUE(recorder.valid());
            ASSERT_EQ(0, recorder.expose("var1", ""));
            for (size_t i = 0; i < 100; ++i) {
                recorder << 2;
            }
            ASSERT_EQ(2l, (int64_t) recorder.average());
            ASSERT_EQ("2", melon::variable_base::describe_exposed("var1"));
            std::vector<std::string> vars;
            melon::variable_base::list_exposed(&vars);
            ASSERT_EQ(1UL, vars.size()) << melon::string_join(vars, ",");
            ASSERT_EQ("var1", vars[0]);
            ASSERT_EQ(1UL, melon::variable_base::count_exposed());
        }
        ASSERT_EQ(0UL, melon::variable_base::count_exposed());
    }

    TEST(RecorderTest, window) {
        melon::IntRecorder c1;
        ASSERT_TRUE(c1.valid());
        melon::window<melon::IntRecorder> w1(&c1, 1);
        melon::window<melon::IntRecorder> w2(&c1, 2);
        melon::window<melon::IntRecorder> w3(&c1, 3);

        const int N = 10000;
        int64_t last_time = melon::get_current_time_micros();
        for (int i = 1; i <= N; ++i) {
            c1 << i;
            int64_t now = melon::get_current_time_micros();
            if (now - last_time >= 1000000L) {
                last_time = now;
                MELON_LOG(INFO) << "c1=" << c1 << " w1=" << w1 << " w2=" << w2 << " w3=" << w3;
            } else {
                usleep(950);
            }
        }
    }

    TEST(RecorderTest, negative) {
        melon::IntRecorder recorder;
        ASSERT_TRUE(recorder.valid());
        for (size_t i = 0; i < 3; ++i) {
            recorder << -2;
        }
        ASSERT_EQ(-2, recorder.average());
    }

    TEST(RecorderTest, positive_overflow) {
        melon::IntRecorder recorder1;
        ASSERT_TRUE(recorder1.valid());
        for (int i = 0; i < 5; ++i) {
            recorder1 << std::numeric_limits<int64_t>::max();
        }
        ASSERT_EQ(std::numeric_limits<int>::max(), recorder1.average());

        melon::IntRecorder recorder2;
        ASSERT_TRUE(recorder2.valid());
        recorder2.set_debug_name("recorder2");
        for (int i = 0; i < 5; ++i) {
            recorder2 << std::numeric_limits<int64_t>::max();
        }
        ASSERT_EQ(std::numeric_limits<int>::max(), recorder2.average());

        melon::IntRecorder recorder3;
        ASSERT_TRUE(recorder3.valid());
        recorder3.expose("recorder3", "");
        for (int i = 0; i < 5; ++i) {
            recorder3 << std::numeric_limits<int64_t>::max();
        }
        ASSERT_EQ(std::numeric_limits<int>::max(), recorder3.average());

        melon::LatencyRecorder latency1;
        latency1.expose("latency1", "");
        latency1 << std::numeric_limits<int64_t>::max();

        melon::LatencyRecorder latency2;
        latency2 << std::numeric_limits<int64_t>::max();
    }

    TEST(RecorderTest, negtive_overflow) {
        melon::IntRecorder recorder1;
        ASSERT_TRUE(recorder1.valid());
        for (int i = 0; i < 5; ++i) {
            recorder1 << std::numeric_limits<int64_t>::min();
        }
        ASSERT_EQ(std::numeric_limits<int>::min(), recorder1.average());

        melon::IntRecorder recorder2;
        ASSERT_TRUE(recorder2.valid());
        recorder2.set_debug_name("recorder2");
        for (int i = 0; i < 5; ++i) {
            recorder2 << std::numeric_limits<int64_t>::min();
        }
        ASSERT_EQ(std::numeric_limits<int>::min(), recorder2.average());

        melon::IntRecorder recorder3;
        ASSERT_TRUE(recorder3.valid());
        recorder3.expose("recorder3", "");
        for (int i = 0; i < 5; ++i) {
            recorder3 << std::numeric_limits<int64_t>::min();
        }
        ASSERT_EQ(std::numeric_limits<int>::min(), recorder3.average());

        melon::LatencyRecorder latency1;
        latency1.expose("latency1", "");
        latency1 << std::numeric_limits<int64_t>::min();

        melon::LatencyRecorder latency2;
        latency2 << std::numeric_limits<int64_t>::min();
    }

    const size_t OPS_PER_THREAD = 20000000;

    static void *thread_counter(void *arg) {
        melon::IntRecorder *recorder = (melon::IntRecorder *) arg;
        melon::stop_watcher timer;
        timer.start();
        for (int i = 0; i < (int) OPS_PER_THREAD; ++i) {
            *recorder << i;
        }
        timer.stop();
        return (void *) (timer.n_elapsed());
    }

    TEST(RecorderTest, perf) {
        melon::IntRecorder recorder;
        ASSERT_TRUE(recorder.valid());
        pthread_t threads[8];
        for (size_t i = 0; i < MELON_ARRAY_SIZE(threads); ++i) {
            pthread_create(&threads[i], NULL, &thread_counter, (void *) &recorder);
        }
        long totol_time = 0;
        for (size_t i = 0; i < MELON_ARRAY_SIZE(threads); ++i) {
            void *ret;
            pthread_join(threads[i], &ret);
            totol_time += (long) ret;
        }
        ASSERT_EQ(((int64_t) OPS_PER_THREAD - 1) / 2, recorder.average());
        MELON_LOG(INFO) << "Recorder takes " << totol_time / (OPS_PER_THREAD * MELON_ARRAY_SIZE(threads))
                        << "ns per sample with " << MELON_ARRAY_SIZE(threads)
                        << " threads";
    }
} // namespace
