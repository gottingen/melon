// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//



#ifndef MELON_RPC_ADAPTIVE_CONNECTION_TYPE_H_
#define MELON_RPC_ADAPTIVE_CONNECTION_TYPE_H_



#include "melon/utility/strings/string_piece.h"
#include "melon/proto/rpc/options.pb.h"

namespace melon {

// Convert a case-insensitive string to corresponding ConnectionType
// Possible options are: short, pooled, single
// Returns: CONNECTION_TYPE_UNKNOWN on error.
ConnectionType StringToConnectionType(const mutil::StringPiece& type,
                                      bool print_log_on_unknown);
inline ConnectionType StringToConnectionType(const mutil::StringPiece& type)
{ return StringToConnectionType(type, true); }

// Convert a ConnectionType to a c-style string.
const char* ConnectionTypeToString(ConnectionType);

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
    void operator=(const mutil::StringPiece& name);

    operator ConnectionType() const { return _type; }
    const char* name() const { return ConnectionTypeToString(_type); }
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


#endif  // MELON_RPC_ADAPTIVE_CONNECTION_TYPE_H_
