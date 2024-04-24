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

#pragma once

#include "melon/rpc/concurrency_limiter.h"

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
    MELON_CACHELINE_ALIGNMENT mutil::atomic<int64_t> _last_sampling_time_us;
    mutil::Mutex _sw_mutex;
    SampleWindow _sw;
    int64_t _timeout_ms;
    int _max_concurrency;
};

}  // namespace policy
}  // namespace melon
