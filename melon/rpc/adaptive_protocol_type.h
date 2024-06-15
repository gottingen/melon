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

    // NOTE: impl. are in melon/protocol.cpp

    // Convert a case-insensitive string to corresponding ProtocolType which is
    // defined in src/melon/options.proto
    // Returns: PROTOCOL_UNKNOWN on error.
    ProtocolType StringToProtocolType(const std::string_view &type,
                                      bool print_log_on_unknown);

    inline ProtocolType StringToProtocolType(const std::string_view &type) {
        return StringToProtocolType(type, true);
    }

    // Convert a ProtocolType to a c-style string.
    const char *ProtocolTypeToString(ProtocolType);

    // Assignable by both ProtocolType and names.
    class AdaptiveProtocolType {
    public:
        explicit AdaptiveProtocolType() : _type(PROTOCOL_UNKNOWN) {}

        explicit AdaptiveProtocolType(ProtocolType type) : _type(type) {}

        explicit AdaptiveProtocolType(std::string_view name) { *this = name; }

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
                param = name.substr(pos + 1);
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

} // namespace melon
