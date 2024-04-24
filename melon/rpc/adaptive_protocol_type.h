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


#ifndef MELON_RPC_ADAPTIVE_PROTOCOL_TYPE_H_
#define MELON_RPC_ADAPTIVE_PROTOCOL_TYPE_H_

#include "melon/utility/strings/string_piece.h"
#include "melon/proto/rpc/options.pb.h"

namespace melon {

// NOTE: impl. are in melon/protocol.cpp

// Convert a case-insensitive string to corresponding ProtocolType which is
// defined in src/melon/options.proto
// Returns: PROTOCOL_UNKNOWN on error.
ProtocolType StringToProtocolType(const mutil::StringPiece& type,
                                  bool print_log_on_unknown);
inline ProtocolType StringToProtocolType(const mutil::StringPiece& type)
{ return StringToProtocolType(type, true); }

// Convert a ProtocolType to a c-style string.
const char* ProtocolTypeToString(ProtocolType);

// Assignable by both ProtocolType and names.
class AdaptiveProtocolType {
public:
    explicit AdaptiveProtocolType() : _type(PROTOCOL_UNKNOWN) {}
    explicit AdaptiveProtocolType(ProtocolType type) : _type(type) {}
    explicit AdaptiveProtocolType(mutil::StringPiece name) { *this = name; }
    ~AdaptiveProtocolType() {}

    void operator=(ProtocolType type) {
        _type = type;
        _name.clear();
        _param.clear();
    }

    void operator=(mutil::StringPiece name) {
        mutil::StringPiece param;
        const size_t pos = name.find(':');
        if (pos != mutil::StringPiece::npos) {
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

    const char* name() const {
        return _name.empty() ? ProtocolTypeToString(_type) : _name.c_str();
    }

    bool has_param() const { return !_param.empty(); }
    const std::string& param() const { return _param; }

private:
    ProtocolType _type;
    std::string _name;
    std::string _param;
};

} // namespace melon

#endif  // MELON_RPC_ADAPTIVE_PROTOCOL_TYPE_H_
