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

#pragma once

#include <melon/rpc/concurrency_limiter.h>

namespace melon {
namespace policy {

class TimeoutConcurrencyLimiter : public ConcurrencyLimiter {
   public:
    TimeoutConcurrencyLimiter();
    explicit TimeoutConcurrencyLimiter(const TimeoutConcurrencyConf& conf);

    bool OnRequested(int current_concurrency, Controller* cntl) override;

    void OnResponded(int error_code, int64_t latency_us) override;

    int MaxConcurrency() override;

    TimeoutConcurrencyLimiter* New(
        const AdaptiveMaxConcurrency&) const override;

   private:
    struct SampleWindow {
        SampleWindow()
            : start_time_us(0),
              succ_count(0),
              failed_count(0),
              total_failed_us(0),
              total_succ_us(0) {}
        int64_t start_time_us;
        int32_t succ_count;
        int32_t failed_count;
        int64_t total_failed_us;
        int64_t total_succ_us;
    };

    bool AddSample(int error_code, int64_t latency_us,
                   int64_t sampling_time_us);

    // The following methods are not thread safe and can only be called
    // in AppSample()
    void ResetSampleWindow(int64_t sampling_time_us);
    void UpdateAvgLatency();
    void AdjustAvgLatency(int64_t avg_latency_us);

    // modified per sample-window or more
    int64_t _avg_latency_us;
    // modified per sample.
    MELON_CACHELINE_ALIGNMENT std::atomic<int64_t> _last_sampling_time_us;
    mutil::Mutex _sw_mutex;
    SampleWindow _sw;
    int64_t _timeout_ms;
    int _max_concurrency;
};

}  // namespace policy
}  // namespace melon
