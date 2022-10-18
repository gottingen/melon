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


#ifndef  MELON_RPC_METHOD_STATUS_H_
#define  MELON_RPC_METHOD_STATUS_H_

#include "melon/base/profile.h"                  // MELON_DISALLOW_COPY_AND_ASSIGN
#include "melon/metrics/all.h"                    // vars
#include "melon/rpc/describable.h"
#include "melon/rpc/concurrency_limiter.h"


namespace melon::rpc {

    class Controller;

    class Server;

    // Record accessing stats of a method.
    class MethodStatus : public Describable {
    public:
        MethodStatus();

        ~MethodStatus();

        // Call this function when the method is about to be called.
        // Returns false when the method is overloaded. If rejected_cc is not
        // nullptr, it's set with the rejected concurrency.
        bool OnRequested(int *rejected_cc = nullptr);

        // Call this when the method just finished.
        // `error_code' : The error code obtained from the controller. Equal to
        // 0 when the call is successful.
        // `latency_us' : microseconds taken by a successful call. Latency can
        // be measured in this utility class as well, but the callsite often
        // did the time keeping and the cost is better saved.
        void OnResponded(int error_code, int64_t latency_us);

        // Expose internal vars.
        // Return 0 on success, -1 otherwise.
        int Expose(const std::string_view &prefix);

        // Describe internal vars, used by /status
        void Describe(std::ostream &os, const DescribeOptions &) const override;

        // Current max_concurrency of the method.
        int MaxConcurrency() const { return _cl ? _cl->MaxConcurrency() : 0; }

    private:
        friend class Server;
        MELON_DISALLOW_COPY_AND_ASSIGN(MethodStatus);

        // Note: SetConcurrencyLimiter() is not thread safe and can only be called
        // before the server is started.
        void SetConcurrencyLimiter(ConcurrencyLimiter *cl);

        melon::histogram _his_latency;
        melon::counter<int64_t> _qps;
        std::unique_ptr<ConcurrencyLimiter> _cl;
        std::atomic<int> _nconcurrency;
        melon::gauge<int64_t> _nerror_var;
        melon::LatencyRecorder _latency_rec;
        melon::status_gauge<int> _nconcurrency_var;
        melon::per_second<melon::gauge<int64_t>> _eps_var;
        melon::status_gauge<int32_t> _max_concurrency_var;
    };

    class ConcurrencyRemover {
    public:
        ConcurrencyRemover(MethodStatus *status, Controller *c, int64_t received_us)
                : _status(status), _c(c), _received_us(received_us) {}

        ~ConcurrencyRemover();

    private:
        MELON_DISALLOW_COPY_AND_ASSIGN(ConcurrencyRemover);

        MethodStatus *_status;
        Controller *_c;
        uint64_t _received_us;
    };

    inline bool MethodStatus::OnRequested(int *rejected_cc) {
        const int cc = _nconcurrency.fetch_add(1, std::memory_order_relaxed) + 1;
        if (nullptr == _cl || _cl->OnRequested(cc)) {
            return true;
        }
        if (rejected_cc) {
            *rejected_cc = cc;
        }
        return false;
    }

    inline void MethodStatus::OnResponded(int error_code, int64_t latency) {
        _nconcurrency.fetch_sub(1, std::memory_order_relaxed);
        if (0 == error_code) {
            _latency_rec << latency;
            _his_latency << static_cast<double>(latency/1000);
            _qps<<1;
        } else {
            _nerror_var << 1;
        }
        if (nullptr != _cl) {
            _cl->OnResponded(error_code, latency);
        }
    }

} // namespace melon::rpc

#endif  // MELON_RPC_METHOD_STATUS_H_
