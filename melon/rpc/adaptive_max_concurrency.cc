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

#include <cstring>
#include <strings.h>
#include "melon/strings/utility.h"
#include "melon/strings/str_format.h"
#include "melon/strings/numbers.h"
#include "melon/log/logging.h"
#include "melon/rpc/adaptive_max_concurrency.h"

namespace melon::rpc {

    AdaptiveMaxConcurrency::AdaptiveMaxConcurrency()
            : _value(UNLIMITED()), _max_concurrency(0) {
    }

    AdaptiveMaxConcurrency::AdaptiveMaxConcurrency(int max_concurrency)
            : _max_concurrency(0) {
        if (max_concurrency <= 0) {
            _value = UNLIMITED();
            _max_concurrency = 0;
        } else {
            _value = melon::string_printf("%d", max_concurrency);
            _max_concurrency = max_concurrency;
        }
    }

    inline bool CompareStringPieceWithoutCase(
            const std::string_view &s1, const char *s2) {
        MELON_DCHECK(s2 != nullptr);
        if (std::strlen(s2) != s1.size()) {
            return false;
        }
        return ::strncasecmp(s1.data(), s2, s1.size()) == 0;
    }

    AdaptiveMaxConcurrency::AdaptiveMaxConcurrency(const std::string_view &value)
            : _max_concurrency(0) {
        int64_t max_concurrency;
        if (melon::simple_atoi(value, &max_concurrency)) {
            operator=(max_concurrency);
        } else {
            melon::copy_to_string(value, &_value);
            _max_concurrency = -1;
        }
    }

    void AdaptiveMaxConcurrency::operator=(const std::string_view &value) {
        int64_t max_concurrency;
        if (melon::simple_atoi(value, &max_concurrency)) {
            return operator=(max_concurrency);
        } else {
            melon::copy_to_string(value, &_value);
            _max_concurrency = -1;
        }
    }

    void AdaptiveMaxConcurrency::operator=(int max_concurrency) {
        if (max_concurrency <= 0) {
            _value = UNLIMITED();
            _max_concurrency = 0;
        } else {
            _value = melon::string_printf("%d", max_concurrency);
            _max_concurrency = max_concurrency;
        }
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
                    const std::string_view &concurrency) {
        return CompareStringPieceWithoutCase(concurrency,
                                             adaptive_concurrency.value().c_str());
    }

}  // namespace melon::rpc