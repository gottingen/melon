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

#include <melon/utility/macros.h>                  // DISALLOW_COPY_AND_ASSIGN
#include <melon/var/var.h>                    // vars
#include <melon/rpc/describable.h>
#include <melon/rpc/concurrency_limiter.h>


namespace melon {

class Controller;
class Server;
// Record accessing stats of a method.
class MethodStatus : public Describable {
public:
    MethodStatus();
    ~MethodStatus();

    // Call this function when the method is about to be called.
    // Returns false when the method is overloaded. If rejected_cc is not
    // NULL, it's set with the rejected concurrency.
    bool OnRequested(int* rejected_cc = NULL, Controller* cntl = NULL);

    // Call this when the method just finished.
    // `error_code' : The error code obtained from the controller. Equal to 
    // 0 when the call is successful.
    // `latency_us' : microseconds taken by a successful call. Latency can
    // be measured in this utility class as well, but the callsite often
    // did the time keeping and the cost is better saved. 
    void OnResponded(int error_code, int64_t latency_us);

    // Expose internal vars.
    // Return 0 on success, -1 otherwise.
    int Expose(const mutil::StringPiece& prefix);

    // Describe internal vars, used by /status
    void Describe(std::ostream &os, const DescribeOptions&) const override;

    // Current max_concurrency of the method.
    int MaxConcurrency() const { return _cl ? _cl->MaxConcurrency() : 0; }

private:
friend class Server;
    DISALLOW_COPY_AND_ASSIGN(MethodStatus);

    // Note: SetConcurrencyLimiter() is not thread safe and can only be called 
    // before the server is started. 
    void SetConcurrencyLimiter(ConcurrencyLimiter* cl);

    std::unique_ptr<ConcurrencyLimiter> _cl;
    mutil::atomic<int> _nconcurrency;
    melon::var::Adder<int64_t>  _nerror_var;
    melon::var::LatencyRecorder _latency_rec;
    melon::var::PassiveStatus<int>  _nconcurrency_var;
    melon::var::PerSecond<melon::var::Adder<int64_t>> _eps_var;
    melon::var::PassiveStatus<int32_t> _max_concurrency_var;
};

class ConcurrencyRemover {
public:
    ConcurrencyRemover(MethodStatus* status, Controller* c, int64_t received_us)
        : _status(status) 
        , _c(c)
        , _received_us(received_us) {}
    ~ConcurrencyRemover();
private:
    DISALLOW_COPY_AND_ASSIGN(ConcurrencyRemover);
    MethodStatus* _status;
    Controller* _c;
    uint64_t _received_us;
};

inline bool MethodStatus::OnRequested(int* rejected_cc, Controller* cntl) {
    const int cc = _nconcurrency.fetch_add(1, mutil::memory_order_relaxed) + 1;
    if (NULL == _cl || _cl->OnRequested(cc, cntl)) {
        return true;
    } 
    if (rejected_cc) {
        *rejected_cc = cc;
    }
    return false;
}

inline void MethodStatus::OnResponded(int error_code, int64_t latency) {
    _nconcurrency.fetch_sub(1, mutil::memory_order_relaxed);
    if (0 == error_code) {
        _latency_rec << latency;
    } else {
        _nerror_var << 1;
    }
    if (NULL != _cl) {
        _cl->OnResponded(error_code, latency);
    }
}

} // namespace melon
