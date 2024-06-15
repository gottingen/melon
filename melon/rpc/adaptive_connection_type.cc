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



#include <turbo/log/logging.h>
#include <melon/rpc/adaptive_connection_type.h>


namespace melon {

    inline bool CompareStringPieceWithoutCase(
            const std::string_view &s1, const char *s2) {
        if (strlen(s2) != s1.size()) {
            return false;
        }
        return strncasecmp(s1.data(), s2, s1.size()) == 0;
    }

    ConnectionType StringToConnectionType(const std::string_view &type,
                                          bool print_log_on_unknown) {
        if (CompareStringPieceWithoutCase(type, "single")) {
            return CONNECTION_TYPE_SINGLE;
        } else if (CompareStringPieceWithoutCase(type, "pooled")) {
            return CONNECTION_TYPE_POOLED;
        } else if (CompareStringPieceWithoutCase(type, "short")) {
            return CONNECTION_TYPE_SHORT;
        }
        LOG_IF(ERROR, print_log_on_unknown && !type.empty())
        << "Unknown connection_type `" << type
        << "', supported types: single pooled short";
        return CONNECTION_TYPE_UNKNOWN;
    }

    const char *ConnectionTypeToString(ConnectionType type) {
        switch (type) {
            case CONNECTION_TYPE_UNKNOWN:
                return "unknown";
            case CONNECTION_TYPE_SINGLE:
                return "single";
            case CONNECTION_TYPE_POOLED:
                return "pooled";
            case CONNECTION_TYPE_SHORT:
                return "short";
        }
        return "unknown";
    }

    void AdaptiveConnectionType::operator=(const std::string_view &name) {
        if (name.empty()) {
            _type = CONNECTION_TYPE_UNKNOWN;
            _error = false;
        } else {
            _type = StringToConnectionType(name);
            _error = (_type == CONNECTION_TYPE_UNKNOWN);
        }
    }

} // namespace melon
