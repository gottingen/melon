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


#include "melon/rpc/policy/timeout_concurrency_limiter.h"
#include "melon/utility/time.h"
#include "melon/fiber/fiber.h"
#include <gtest/gtest.h>

namespace melon {
namespace policy {
DECLARE_int32(timeout_cl_sample_window_size_ms);
DECLARE_int32(timeout_cl_min_sample_count);
DECLARE_int32(timeout_cl_max_sample_count);
}  // namespace policy
}  // namespace melon

TEST(TimeoutConcurrencyLimiterTest, AddSample) {
    {
        melon::policy::FLAGS_timeout_cl_sample_window_size_ms = 10;
        melon::policy::FLAGS_timeout_cl_min_sample_count = 5;
        melon::policy::FLAGS_timeout_cl_max_sample_count = 10;

        melon::policy::TimeoutConcurrencyLimiter limiter;
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        fiber_usleep(10 * 1000);
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        ASSERT_EQ(limiter._sw.succ_count, 0);
        ASSERT_EQ(limiter._sw.failed_count, 0);

        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        fiber_usleep(10 * 1000);
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        ASSERT_EQ(limiter._sw.succ_count, 0);
        ASSERT_EQ(limiter._sw.failed_count, 0);
        ASSERT_EQ(limiter._avg_latency_us, 50);

        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        ASSERT_EQ(limiter._sw.succ_count, 0);
        ASSERT_EQ(limiter._sw.failed_count, 0);
        ASSERT_EQ(limiter._avg_latency_us, 50);

        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        ASSERT_EQ(limiter._sw.succ_count, 6);
        ASSERT_EQ(limiter._sw.failed_count, 0);

        limiter.ResetSampleWindow(mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(0, 50, mutil::gettimeofday_us());
        limiter.AddSample(1, 50, mutil::gettimeofday_us());
        limiter.AddSample(1, 50, mutil::gettimeofday_us());
        limiter.AddSample(1, 50, mutil::gettimeofday_us());
        ASSERT_EQ(limiter._sw.succ_count, 3);
        ASSERT_EQ(limiter._sw.failed_count, 3);
    }
}

TEST(TimeoutConcurrencyLimiterTest, OnResponded) {
    melon::policy::FLAGS_timeout_cl_sample_window_size_ms = 10;
    melon::policy::FLAGS_timeout_cl_min_sample_count = 5;
    melon::policy::FLAGS_timeout_cl_max_sample_count = 10;
    melon::policy::TimeoutConcurrencyLimiter limiter;
    limiter.OnResponded(0, 50);
    limiter.OnResponded(0, 50);
    fiber_usleep(100);
    limiter.OnResponded(0, 50);
    limiter.OnResponded(1, 50);
    ASSERT_EQ(limiter._sw.succ_count, 2);
    ASSERT_EQ(limiter._sw.failed_count, 0);
}

TEST(TimeoutConcurrencyLimiterTest, AdaptiveMaxConcurrencyTest) {
    {
        melon::AdaptiveMaxConcurrency concurrency(
            melon::TimeoutConcurrencyConf{100, 100});
        ASSERT_EQ(concurrency.type(), "timeout");
        ASSERT_EQ(concurrency.value(), "timeout");
    }
    {
        melon::AdaptiveMaxConcurrency concurrency;
        concurrency = "timeout";
        ASSERT_EQ(concurrency.type(), "timeout");
        ASSERT_EQ(concurrency.value(), "timeout");
    }
    {
        melon::AdaptiveMaxConcurrency concurrency;
        concurrency = melon::TimeoutConcurrencyConf{50, 100};
        ASSERT_EQ(concurrency.type(), "timeout");
        ASSERT_EQ(concurrency.value(), "timeout");
        auto time_conf = static_cast<melon::TimeoutConcurrencyConf>(concurrency);
        ASSERT_EQ(time_conf.timeout_ms, 50);
        ASSERT_EQ(time_conf.max_concurrency, 100);
    }
}
