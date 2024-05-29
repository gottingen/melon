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


#include <cstring>
#include <strings.h>
#include <melon/utility/string_printf.h>
#include <turbo/log/logging.h>
#include <melon/utility/strings/string_number_conversions.h>
#include <melon/rpc/adaptive_max_concurrency.h>

namespace melon {

    AdaptiveMaxConcurrency::AdaptiveMaxConcurrency()
            : _value(UNLIMITED()), _max_concurrency(0) {
    }

    AdaptiveMaxConcurrency::AdaptiveMaxConcurrency(int max_concurrency)
            : _max_concurrency(0) {
        if (max_concurrency <= 0) {
            _value = UNLIMITED();
            _max_concurrency = 0;
        } else {
            _value = mutil::string_printf("%d", max_concurrency);
            _max_concurrency = max_concurrency;
        }
    }

    AdaptiveMaxConcurrency::AdaptiveMaxConcurrency(
            const TimeoutConcurrencyConf &value)
            : _value("timeout"), _max_concurrency(-1), _timeout_conf(value) {}

    inline bool CompareStringPieceWithoutCase(
            const mutil::StringPiece &s1, const char *s2) {
        DCHECK(s2 != NULL);
        if (std::strlen(s2) != s1.size()) {
            return false;
        }
        return ::strncasecmp(s1.data(), s2, s1.size()) == 0;
    }

    AdaptiveMaxConcurrency::AdaptiveMaxConcurrency(const mutil::StringPiece &value)
            : _max_concurrency(0) {
        int max_concurrency = 0;
        if (mutil::StringToInt(value, &max_concurrency)) {
            operator=(max_concurrency);
        } else {
            value.CopyToString(&_value);
            _max_concurrency = -1;
        }
    }

    void AdaptiveMaxConcurrency::operator=(const mutil::StringPiece &value) {
        int max_concurrency = 0;
        if (mutil::StringToInt(value, &max_concurrency)) {
            return operator=(max_concurrency);
        } else {
            value.CopyToString(&_value);
            _max_concurrency = -1;
        }
    }

    void AdaptiveMaxConcurrency::operator=(int max_concurrency) {
        if (max_concurrency <= 0) {
            _value = UNLIMITED();
            _max_concurrency = 0;
        } else {
            _value = mutil::string_printf("%d", max_concurrency);
            _max_concurrency = max_concurrency;
        }
    }

    void AdaptiveMaxConcurrency::operator=(const TimeoutConcurrencyConf &value) {
        _value = "timeout";
        _max_concurrency = -1;
        _timeout_conf = value;
    }

    const std::string &AdaptiveMaxConcurrency::type() const {
        if (_max_concurrency > 0) {
            return CONSTANT();
        } else if (_max_concurrency == 0) {
            return UNLIMITED();
        } else {
            return _value;
        }
    }

    const std::string &AdaptiveMaxConcurrency::UNLIMITED() {
        static std::string *s = new std::string("unlimited");
        return *s;
    }

    const std::string &AdaptiveMaxConcurrency::CONSTANT() {
        static std::string *s = new std::string("constant");
        return *s;
    }

    bool operator==(const AdaptiveMaxConcurrency &adaptive_concurrency,
                    const mutil::StringPiece &concurrency) {
        return CompareStringPieceWithoutCase(concurrency,
                                             adaptive_concurrency.value().c_str());
    }

}  // namespace melon
