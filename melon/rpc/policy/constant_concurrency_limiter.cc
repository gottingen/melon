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


#include <melon/rpc/policy/constant_concurrency_limiter.h>

namespace melon {
namespace policy {

ConstantConcurrencyLimiter::ConstantConcurrencyLimiter(int max_concurrency)
    : _max_concurrency(max_concurrency) {
}

bool ConstantConcurrencyLimiter::OnRequested(int current_concurrency, Controller*) {
    return current_concurrency <= _max_concurrency;
}

void ConstantConcurrencyLimiter::OnResponded(int error_code, int64_t latency) {
}

int ConstantConcurrencyLimiter::MaxConcurrency() {
    return _max_concurrency.load(mutil::memory_order_relaxed);
}

ConstantConcurrencyLimiter*
ConstantConcurrencyLimiter::New(const AdaptiveMaxConcurrency& amc) const {
    MCHECK_EQ(amc.type(), AdaptiveMaxConcurrency::CONSTANT());
    return new ConstantConcurrencyLimiter(static_cast<int>(amc));
}

}  // namespace policy
}  // namespace melon
