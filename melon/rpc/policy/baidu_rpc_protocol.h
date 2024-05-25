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

// Parse binary format of melon_std
ParseResult ParseBRPCMessage(mutil::IOBuf* source, Socket *socket, bool read_eof,
                            const void *arg);

// Actions to a (client) request in melon_std format
void ProcessBRPCRequest(InputMessageBase* msg);

// Actions to a (server) response in melon_std format.
void ProcessBRPCResponse(InputMessageBase* msg);

// Verify authentication information in melon_std format
bool VerifyBRPCRequest(const InputMessageBase* msg);

// Pack `request' to `method' into `buf'.
void PackBRPCRequest(mutil::IOBuf* buf,
                    SocketMessage**,
                    uint64_t correlation_id,
                    const google::protobuf::MethodDescriptor* method,
                    Controller* controller,
                    const mutil::IOBuf& request,
                    const Authenticator* auth);

}  // namespace policy
} // namespace melon
