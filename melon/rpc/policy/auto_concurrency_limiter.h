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

#include <melon/var/var.h>
#include <melon/utility/containers/bounded_queue.h>
#include <melon/rpc/concurrency_limiter.h>

namespace melon {
namespace policy {

class AutoConcurrencyLimiter : public ConcurrencyLimiter {
public:
    AutoConcurrencyLimiter();

    bool OnRequested(int current_concurrency, Controller*) override;
    
    void OnResponded(int error_code, int64_t latency_us) override;

    int MaxConcurrency() override;

    AutoConcurrencyLimiter* New(const AdaptiveMaxConcurrency&) const override;

private:
    struct SampleWindow {
        SampleWindow() 
            : start_time_us(0)
            , succ_count(0)
            , failed_count(0)
            , total_failed_us(0)
            , total_succ_us(0) {}
        int64_t start_time_us;
        int32_t succ_count;
        int32_t failed_count;
        int64_t total_failed_us;
        int64_t total_succ_us;
    };

    bool AddSample(int error_code, int64_t latency_us, int64_t sampling_time_us);
    int64_t NextResetTime(int64_t sampling_time_us);

    // The following methods are not thread safe and can only be called 
    // in AppSample()
    void UpdateMaxConcurrency(int64_t sampling_time_us);
    void ResetSampleWindow(int64_t sampling_time_us);
    void UpdateMinLatency(int64_t latency_us);
    void UpdateQps(double qps);

    void AdjustMaxConcurrency(int next_max_concurrency);

    // modified per sample-window or more
    int _max_concurrency;
    int64_t _remeasure_start_us;
    int64_t _reset_latency_us;
    int64_t _min_latency_us; 
    double _ema_max_qps;
    double _explore_ratio;
  
    // modified per sample.
    MELON_CACHELINE_ALIGNMENT mutil::atomic<int64_t> _last_sampling_time_us;
    mutil::Mutex _sw_mutex;
    SampleWindow _sw;

    // modified per request.
    MELON_CACHELINE_ALIGNMENT mutil::atomic<int32_t> _total_succ_req;
};

}  // namespace policy
}  // namespace melon

