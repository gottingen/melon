// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "info_thread.h"

namespace melon::rpc {

InfoThread::InfoThread()
    : _stop(false)
    , _tid(0) {
    pthread_mutex_init(&_mutex, nullptr);
    pthread_cond_init(&_cond, nullptr);
}

InfoThread::~InfoThread() {
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);
}

void InfoThread::run() {
    int64_t i = 0;
    int64_t last_sent_count = 0;
    int64_t last_succ_count = 0;
    int64_t last_error_count = 0;
    int64_t start_time = melon::get_current_time_micros();
    while (!_stop) {
        int64_t end_time = 0;
        while (!_stop &&
               (end_time = melon::get_current_time_micros()) < start_time + 1000000L) {
            MELON_SCOPED_LOCK(_mutex);
            if (!_stop) {
                timespec ts = melon::time_point::from_unix_micros(end_time).to_timespec();
                pthread_cond_timedwait(&_cond, &_mutex, &ts);
            }
        }
        start_time = melon::get_current_time_micros();
        char buf[64];
        const time_t tm_s = start_time / 1000000L;
        struct tm lt;
        strftime(buf, sizeof(buf), "%Y/%m/%d-%H:%M:%S",
                 localtime_r(&tm_s, &lt));

        const int64_t cur_sent_count = _options.sent_count->get_value();
        const int64_t cur_succ_count = _options.latency_recorder->count();
        const int64_t cur_error_count = _options.error_count->get_value();
        printf("%s\tsent:%-10dsuccess:%-10derror:%-6dtotal_error:%-10lldtotal_sent:%-10lld\n",
               buf,
               (int)(cur_sent_count - last_sent_count),
               (int)(cur_succ_count - last_succ_count),
               (int)(cur_error_count - last_error_count),
               (long long)cur_error_count,
               (long long)cur_sent_count);
        last_sent_count = cur_sent_count;
        last_succ_count = cur_succ_count;
        last_error_count = cur_error_count;

        if (_stop || ++i % 10 == 0) {
            printf("[Latency]\n"
                   "  avg     %10lld us\n"
                   "  50%%     %10lld us\n"
                   "  70%%     %10lld us\n"
                   "  90%%     %10lld us\n"
                   "  95%%     %10lld us\n"
                   "  97%%     %10lld us\n"
                   "  99%%     %10lld us\n"
                   "  99.9%%   %10lld us\n"
                   "  99.99%%  %10lld us\n"
                   "  max     %10lld us\n",
                   (long long)_options.latency_recorder->latency(),
                   (long long)_options.latency_recorder->latency_percentile(0.5),
                   (long long)_options.latency_recorder->latency_percentile(0.7),
                   (long long)_options.latency_recorder->latency_percentile(0.9),
                   (long long)_options.latency_recorder->latency_percentile(0.95),
                   (long long)_options.latency_recorder->latency_percentile(0.97),
                   (long long)_options.latency_recorder->latency_percentile(0.99),
                   (long long)_options.latency_recorder->latency_percentile(0.999),
                   (long long)_options.latency_recorder->latency_percentile(0.9999),
                   (long long)_options.latency_recorder->max_latency());
        }
    }
}

static void* run_info_thread(void* arg) {
    ((InfoThread*)arg)->run();
    return nullptr;
}

bool InfoThread::start(const InfoThreadOptions& options) {
    if (options.latency_recorder == nullptr ||
        options.error_count == nullptr ||
        options.sent_count == nullptr) {
        MELON_LOG(ERROR) << "Some required options are nullptr";
        return false;
    }
    _options = options;
    _stop = false;
    if (pthread_create(&_tid, nullptr, run_info_thread, this) != 0) {
        MELON_LOG(ERROR) << "Fail to create info_thread";
        return false;
    }
    return true;
}

void InfoThread::stop() {
    {
        MELON_SCOPED_LOCK(_mutex);
        if (_stop) {
            return;
        }
        _stop = true;
        pthread_cond_signal(&_cond);
    }
    pthread_join(_tid, nullptr);
}

} // namespace melon::rpc