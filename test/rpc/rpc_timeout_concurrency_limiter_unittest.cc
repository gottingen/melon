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


#include <melon/rpc/policy/timeout_concurrency_limiter.h>
#include <melon/utility/time.h>
#include <melon/fiber/fiber.h>
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
