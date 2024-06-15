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

#include <atomic>

namespace melon {

    class CircuitBreaker {
    public:
        CircuitBreaker();

        ~CircuitBreaker() {}

        // Sampling the current rpc. Returns false if a node needs to
        // be isolated. Otherwise return true.
        // error_code: Error_code of this call, 0 means success.
        // latency: Time cost of this call.
        // Note: Once OnCallEnd() determined that a node needs to be isolated,
        // it will always return false until you call Reset(). Usually Reset()
        // will be called in the health check thread.
        bool OnCallEnd(int error_code, int64_t latency);

        // Reset CircuitBreaker and clear history data. will erase the historical
        // data and start sampling again. Before you call this method, you need to
        // ensure that no one else is accessing CircuitBreaker.
        void Reset();

        // Mark the Socket as broken. Call this method when you want to isolate a
        // node in advance. When this method is called multiple times in succession,
        // only the first call will take effect.
        void MarkAsBroken();

        // Number of times marked as broken
        int isolated_times() const {
            return _isolated_times.load(std::memory_order_relaxed);
        }

        // The duration that should be isolated when the socket fails in milliseconds.
        // The higher the frequency of socket errors, the longer the duration.
        int isolation_duration_ms() const {
            return _isolation_duration_ms.load(std::memory_order_relaxed);
        }

    private:
        void UpdateIsolationDuration();

        class EmaErrorRecorder {
        public:
            EmaErrorRecorder(int windows_size, int max_error_percent);

            bool OnCallEnd(int error_code, int64_t latency);

            void Reset();

        private:
            int64_t UpdateLatency(int64_t latency);

            bool UpdateErrorCost(int64_t latency, int64_t ema_latency);

            const int _window_size;
            const int _max_error_percent;
            const double _smooth;

            std::atomic<int32_t> _sample_count_when_initializing;
            std::atomic<int32_t> _error_count_when_initializing;
            std::atomic<int64_t> _ema_error_cost;
            std::atomic<int64_t> _ema_latency;
        };

        EmaErrorRecorder _long_window;
        EmaErrorRecorder _short_window;
        int64_t _last_reset_time_ms;
        std::atomic<int> _isolation_duration_ms;
        std::atomic<int> _isolated_times;
        std::atomic<bool> _broken;
    };

}  // namespace melon
