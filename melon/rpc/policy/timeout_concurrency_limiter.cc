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
#include <melon/rpc/controller.h>
#include <melon/proto/rpc/errno.pb.h>
#include <cmath>
#include <turbo/flags/flag.h>


TURBO_FLAG(int32_t, timeout_cl_sample_window_size_ms, 1000,
"Duration of the sampling window.");
TURBO_FLAG(int32_t, timeout_cl_min_sample_count, 100,
"During the duration of the sampling window, if the number of "
"requests collected is less than this value, the sampling window "
"will be discarded.");
TURBO_FLAG(int32_t, timeout_cl_max_sample_count, 200,
"During the duration of the sampling window, once the number of "
"requests collected is greater than this value, even if the "
"duration of the window has not ended, the max_concurrency will "
"be updated and a new sampling window will be started.");
TURBO_FLAG(double, timeout_cl_sampling_interval_ms, 0.1,
"Interval for sampling request in auto concurrency limiter");
TURBO_FLAG(int32_t, timeout_cl_initial_avg_latency_us, 500,
"Initial max concurrency for gradient concurrency limiter");
TURBO_FLAG(bool, 
        timeout_cl_enable_error_punish, true,
"Whether to consider failed requests when calculating maximum concurrency");
TURBO_FLAG(double, 
        timeout_cl_fail_punish_ratio, 1.0,
"Use the failed requests to punish normal requests. The larger "
"the configuration item, the more aggressive the penalty strategy.");
TURBO_FLAG(int32_t, timeout_cl_default_timeout_ms, 500,
"Default timeout for rpc request");
TURBO_FLAG(int32_t, timeout_cl_max_concurrency, 100,
"When average latency statistics not refresh, this flag can keep "
"requests not exceed this max concurrency");

namespace melon {
namespace policy {
    
TimeoutConcurrencyLimiter::TimeoutConcurrencyLimiter()
    : _avg_latency_us(turbo::get_flag(FLAGS_timeout_cl_initial_avg_latency_us)),
      _last_sampling_time_us(0),
      _timeout_ms(turbo::get_flag(FLAGS_timeout_cl_default_timeout_ms)),
      _max_concurrency(turbo::get_flag(FLAGS_timeout_cl_max_concurrency)) {}

TimeoutConcurrencyLimiter::TimeoutConcurrencyLimiter(
    const TimeoutConcurrencyConf &conf)
    : _avg_latency_us(turbo::get_flag(FLAGS_timeout_cl_initial_avg_latency_us)),
      _last_sampling_time_us(0),
      _timeout_ms(conf.timeout_ms),
      _max_concurrency(conf.max_concurrency) {}

TimeoutConcurrencyLimiter *TimeoutConcurrencyLimiter::New(
    const AdaptiveMaxConcurrency &amc) const {
    return new (std::nothrow)
        TimeoutConcurrencyLimiter(static_cast<TimeoutConcurrencyConf>(amc));
}

bool TimeoutConcurrencyLimiter::OnRequested(int current_concurrency,
                                            Controller *cntl) {
    auto timeout_ms = _timeout_ms;
    if (cntl != nullptr && cntl->timeout_ms() != UNSET_MAGIC_NUM) {
        timeout_ms = cntl->timeout_ms();
    }
    // In extreme cases, the average latency may be greater than requested
    // timeout, allow currency_concurrency is 1 ensures the average latency can
    // be obtained renew.
    return current_concurrency == 1 ||
           (current_concurrency <= _max_concurrency &&
            _avg_latency_us < timeout_ms * 1000);
}

void TimeoutConcurrencyLimiter::OnResponded(int error_code,
                                            int64_t latency_us) {
    if (ELIMIT == error_code) {
        return;
    }

    const int64_t now_time_us = mutil::gettimeofday_us();
    int64_t last_sampling_time_us =
        _last_sampling_time_us.load(mutil::memory_order_relaxed);

    if (last_sampling_time_us == 0 ||
        now_time_us - last_sampling_time_us >=
                turbo::get_flag(FLAGS_timeout_cl_sampling_interval_ms) * 1000) {
        bool sample_this_call = _last_sampling_time_us.compare_exchange_strong(
            last_sampling_time_us, now_time_us, mutil::memory_order_relaxed);
        if (sample_this_call) {
            bool sample_window_submitted =
                AddSample(error_code, latency_us, now_time_us);
            if (sample_window_submitted) {
                // The following log prints has data-race in extreme cases,
                // unless you are in debug, you should not open it.
                VLOG(1) << "Sample window submitted, current avg_latency_us:"
                        << _avg_latency_us;
            }
        }
    }
}

int TimeoutConcurrencyLimiter::MaxConcurrency() {
    return turbo::get_flag(FLAGS_timeout_cl_max_concurrency);
}

bool TimeoutConcurrencyLimiter::AddSample(int error_code, int64_t latency_us,
                                          int64_t sampling_time_us) {
    std::unique_lock<mutil::Mutex> lock_guard(_sw_mutex);
    if (_sw.start_time_us == 0) {
        _sw.start_time_us = sampling_time_us;
    }

    if (error_code != 0 && turbo::get_flag(FLAGS_timeout_cl_enable_error_punish)) {
        ++_sw.failed_count;
        _sw.total_failed_us += latency_us;
    } else if (error_code == 0) {
        ++_sw.succ_count;
        _sw.total_succ_us += latency_us;
    }

    if (_sw.succ_count + _sw.failed_count < turbo::get_flag(FLAGS_timeout_cl_min_sample_count)) {
        if (sampling_time_us - _sw.start_time_us >=
                turbo::get_flag(FLAGS_timeout_cl_sample_window_size_ms) * 1000) {
            // If the sample size is insufficient at the end of the sampling
            // window, discard the entire sampling window
            ResetSampleWindow(sampling_time_us);
        }
        return false;
    }
    if (sampling_time_us - _sw.start_time_us <
            turbo::get_flag(FLAGS_timeout_cl_sample_window_size_ms) * 1000 &&
        _sw.succ_count + _sw.failed_count < turbo::get_flag(FLAGS_timeout_cl_max_sample_count)) {
        return false;
    }

    if (_sw.succ_count > 0) {
        UpdateAvgLatency();
    } else {
        // All request failed
        AdjustAvgLatency(_avg_latency_us * 2);
    }
    ResetSampleWindow(sampling_time_us);
    return true;
}

void TimeoutConcurrencyLimiter::ResetSampleWindow(int64_t sampling_time_us) {
    _sw.start_time_us = sampling_time_us;
    _sw.succ_count = 0;
    _sw.failed_count = 0;
    _sw.total_failed_us = 0;
    _sw.total_succ_us = 0;
}

void TimeoutConcurrencyLimiter::AdjustAvgLatency(int64_t avg_latency_us) {
    _avg_latency_us = avg_latency_us;
}

void TimeoutConcurrencyLimiter::UpdateAvgLatency() {
    double failed_punish =
        _sw.total_failed_us * turbo::get_flag(FLAGS_timeout_cl_fail_punish_ratio);
    auto avg_latency_us =
        std::ceil((failed_punish + _sw.total_succ_us) / _sw.succ_count);
    AdjustAvgLatency(avg_latency_us);
}

}  // namespace policy
}  // namespace melon
