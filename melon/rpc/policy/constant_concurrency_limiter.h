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

class ConstantConcurrencyLimiter : public ConcurrencyLimiter {
public:
    explicit ConstantConcurrencyLimiter(int max_concurrency);
    
    bool OnRequested(int current_concurrency, Controller*) override;
    
    void OnResponded(int error_code, int64_t latency_us) override;

    int MaxConcurrency() override;

    ConstantConcurrencyLimiter* New(const AdaptiveMaxConcurrency&) const override;

private:
    mutil::atomic<int> _max_concurrency;
};

}  // namespace policy
}  // namespace melon

