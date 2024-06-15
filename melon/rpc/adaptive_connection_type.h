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

    // Convert a case-insensitive string to corresponding ConnectionType
    // Possible options are: short, pooled, single
    // Returns: CONNECTION_TYPE_UNKNOWN on error.
    ConnectionType StringToConnectionType(const std::string_view &type,
                                          bool print_log_on_unknown);

    inline ConnectionType StringToConnectionType(const std::string_view &type) {
        return StringToConnectionType(type, true);
    }

    // Convert a ConnectionType to a c-style string.
    const char *ConnectionTypeToString(ConnectionType);

    // Assignable by both ConnectionType and names.
    class AdaptiveConnectionType {
    public:
        AdaptiveConnectionType() : _type(CONNECTION_TYPE_UNKNOWN), _error(false) {}

        AdaptiveConnectionType(ConnectionType type) : _type(type), _error(false) {}

        ~AdaptiveConnectionType() {}

        void operator=(ConnectionType type) {
            _type = type;
            _error = false;
        }

        void operator=(const std::string_view &name);

        operator ConnectionType() const { return _type; }

        const char *name() const { return ConnectionTypeToString(_type); }

        bool has_error() const { return _error; }

    private:
        ConnectionType _type;
        // Since this structure occupies 8 bytes in 64-bit machines anyway,
        // we add a field to mark if last operator=(name) failed so that
        // channel can print a error log before re-selecting a valid
        // ConnectionType for user.
        bool _error;
    };

} // namespace melon

