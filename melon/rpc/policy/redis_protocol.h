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

#include <melon/rpc/protocol.h>


namespace melon {
namespace policy {

// Parse redis response.
ParseResult ParseRedisMessage(mutil::IOBuf* source, Socket *socket, bool read_eof,
                              const void *arg);

// Actions to a redis response.
void ProcessRedisResponse(InputMessageBase* msg);

// Actions to a redis request, which is left unimplemented.
// All requests are processed in execution queue pushed in
// the parsing process. This function must be declared since
// server only enables redis as a server-side protocol when
// this function is declared.
void ProcessRedisRequest(InputMessageBase* msg);

// Serialize a redis request.
void SerializeRedisRequest(mutil::IOBuf* buf,
                           Controller* cntl,
                           const google::protobuf::Message* request);

// Pack `request' to `method' into `buf'.
void PackRedisRequest(mutil::IOBuf* buf,
                      SocketMessage**,
                      uint64_t correlation_id,
                      const google::protobuf::MethodDescriptor* method,
                      Controller* controller,
                      const mutil::IOBuf& request,
                      const Authenticator* auth);

const std::string& GetRedisMethodName(
    const google::protobuf::MethodDescriptor*,
    const Controller*);

}  // namespace policy
} // namespace melon
