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


#ifndef MELON_RPC_CONCURRENCY_LIMITER_H_
#define MELON_RPC_CONCURRENCY_LIMITER_H_
                                            
#include "melon/rpc/describable.h"
#include "melon/rpc/destroyable.h"
#include "melon/rpc/extension.h"                       // Extension<T>
#include "melon/rpc/adaptive_max_concurrency.h"        // AdaptiveMaxConcurrency
#include "melon/rpc/controller.h"

namespace melon {

class ConcurrencyLimiter {
public:
    virtual ~ConcurrencyLimiter() {}

    // This method should be called each time a request comes in. It returns
    // false when the concurrency reaches the upper limit, otherwise it 
    // returns true. Normally, when OnRequested returns false, you should 
    // return an ELIMIT error directly.
    virtual bool OnRequested(int current_concurrency, Controller* cntl) = 0;

    // Each request should call this method before responding.
    // `error_code' : Error code obtained from the controller, 0 means success.
    // `latency' : Microseconds taken by RPC.
    // NOTE: Even if OnRequested returns false, after sending ELIMIT, you 
    // still need to call OnResponded.
    virtual void OnResponded(int error_code, int64_t latency_us) = 0;

    // Returns the latest max_concurrency.
    // The return value is only for logging.
    virtual int MaxConcurrency() = 0;

    // Create an instance from the amc
    // Caller is responsible for delete the instance after usage.
    virtual ConcurrencyLimiter* New(const AdaptiveMaxConcurrency& amc) const = 0;
};

inline Extension<const ConcurrencyLimiter>* ConcurrencyLimiterExtension() {
    return Extension<const ConcurrencyLimiter>::instance();
}

}  // namespace melon


#endif // MELON_RPC_CONCURRENCY_LIMITER_H_
