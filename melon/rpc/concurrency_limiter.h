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

#include <melon/rpc/describable.h>
#include <melon/rpc/destroyable.h>
#include <melon/rpc/extension.h>                       // Extension<T>
#include <melon/rpc/adaptive_max_concurrency.h>        // AdaptiveMaxConcurrency
#include <melon/rpc/controller.h>

namespace melon {

    class ConcurrencyLimiter {
    public:
        virtual ~ConcurrencyLimiter() {}

        // This method should be called each time a request comes in. It returns
        // false when the concurrency reaches the upper limit, otherwise it
        // returns true. Normally, when OnRequested returns false, you should
        // return an ELIMIT error directly.
        virtual bool OnRequested(int current_concurrency, Controller *cntl) = 0;

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
        virtual ConcurrencyLimiter *New(const AdaptiveMaxConcurrency &amc) const = 0;
    };

    inline Extension<const ConcurrencyLimiter> *ConcurrencyLimiterExtension() {
        return Extension<const ConcurrencyLimiter>::instance();
    }

}  // namespace melon
