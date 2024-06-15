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

#include <string_view>
#include <melon/proto/rpc/options.pb.h>

namespace melon {

    // timeout concurrency limiter config
    struct TimeoutConcurrencyConf {
        int64_t timeout_ms;
        int max_concurrency;
    };

    class AdaptiveMaxConcurrency {
    public:
        explicit AdaptiveMaxConcurrency();

        explicit AdaptiveMaxConcurrency(int max_concurrency);

        explicit AdaptiveMaxConcurrency(const std::string_view &value);

        explicit AdaptiveMaxConcurrency(const TimeoutConcurrencyConf &value);

        // Non-trivial destructor to prevent AdaptiveMaxConcurrency from being
        // passed to variadic arguments without explicit type conversion.
        // eg:
        // printf("%d", options.max_concurrency)                  // compile error
        // printf("%s", options.max_concurrency.value().c_str()) // ok
        ~AdaptiveMaxConcurrency() {}

        void operator=(int max_concurrency);

        void operator=(const std::string_view &value);

        void operator=(const TimeoutConcurrencyConf &value);

        // 0  for type="unlimited"
        // >0 for type="constant"
        // <0 for type="user-defined"
        operator int() const { return _max_concurrency; }

        operator TimeoutConcurrencyConf() const { return _timeout_conf; }

        // "unlimited" for type="unlimited"
        // "10" "20" "30" for type="constant"
        // "user-defined" for type="user-defined"
        const std::string &value() const { return _value; }

        // "unlimited", "constant" or "user-defined"
        const std::string &type() const;

        // Get strings filled with "unlimited" and "constant"
        static const std::string &UNLIMITED();

        static const std::string &CONSTANT();

    private:
        std::string _value;
        int _max_concurrency;
        TimeoutConcurrencyConf
                _timeout_conf;  // TODO std::varient for different type
    };

    inline std::ostream &operator<<(std::ostream &os, const AdaptiveMaxConcurrency &amc) {
        return os << amc.value();
    }

    bool operator==(const AdaptiveMaxConcurrency &adaptive_concurrency,
                    const std::string_view &concurrency);

    inline bool operator==(const std::string_view &concurrency,
                           const AdaptiveMaxConcurrency &adaptive_concurrency) {
        return adaptive_concurrency == concurrency;
    }

    inline bool operator!=(const AdaptiveMaxConcurrency &adaptive_concurrency,
                           const std::string_view &concurrency) {
        return !(adaptive_concurrency == concurrency);
    }

    inline bool operator!=(const std::string_view &concurrency,
                           const AdaptiveMaxConcurrency &adaptive_concurrency) {
        return !(adaptive_concurrency == concurrency);
    }

}  // namespace melon
