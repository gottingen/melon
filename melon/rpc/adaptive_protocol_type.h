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

#ifndef MELON_RPC_ADAPTIVE_PROTOCOL_TYPE_H_
#define MELON_RPC_ADAPTIVE_PROTOCOL_TYPE_H_

// To melon developers: This is a header included by user, don't depend
// on internal structures, use opaque pointers instead.

#include <string_view>
#include "melon/rpc/options.pb.h"
#include "melon/strings/str_format.h"
#include "melon/strings/safe_substr.h"

namespace melon::rpc {

    // NOTE: impl. are in melon/rpc/protocol.cpp

    // Convert a case-insensitive string to corresponding ProtocolType which is
    // defined in melon/rpc/options.proto
    // Returns: PROTOCOL_UNKNOWN on error.
    ProtocolType StringToProtocolType(const std::string_view &type,
                                      bool print_log_on_unknown);

    inline ProtocolType StringToProtocolType(const std::string_view &type) { return StringToProtocolType(type, true); }

    // Convert a ProtocolType to a c-style string.
    const char *ProtocolTypeToString(ProtocolType);

    // Assignable by both ProtocolType and names.
    class AdaptiveProtocolType {
    public:
        explicit AdaptiveProtocolType() : _type(PROTOCOL_UNKNOWN) {}

        explicit AdaptiveProtocolType(ProtocolType type) : _type(type) {}

        ~AdaptiveProtocolType() {}

        void operator=(ProtocolType type) {
            _type = type;
            _name.clear();
            _param.clear();
        }

        void operator=(std::string_view name) {
            std::string_view param;
            const size_t pos = name.find(':');
            if (pos != std::string_view::npos) {
                param = melon::safe_substr(name, pos + 1);
                name.remove_suffix(name.size() - pos);
            }
            _type = StringToProtocolType(name);
            if (_type == PROTOCOL_UNKNOWN) {
                _name.assign(name.data(), name.size());
            } else {
                _name.clear();
            }
            if (!param.empty()) {
                _param.assign(param.data(), param.size());
            } else {
                _param.clear();
            }
        };

        operator ProtocolType() const { return _type; }

        const char *name() const {
            return _name.empty() ? ProtocolTypeToString(_type) : _name.c_str();
        }

        bool has_param() const { return !_param.empty(); }

        const std::string &param() const { return _param; }

    private:
        ProtocolType _type;
        std::string _name;
        std::string _param;
    };

} // namespace melon::rpc

#endif  // MELON_RPC_ADAPTIVE_PROTOCOL_TYPE_H_
