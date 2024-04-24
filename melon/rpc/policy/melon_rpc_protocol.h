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

#pragma once

#include "melon/rpc/protocol.h"

namespace melon {
namespace policy {

// Parse binary format of melon_std
ParseResult ParseMStdMessage(mutil::IOBuf* source, Socket *socket, bool read_eof,
                            const void *arg);

// Actions to a (client) request in melon_std format
void ProcessMStdRequest(InputMessageBase* msg);

// Actions to a (server) response in melon_std format.
void ProcessMStdResponse(InputMessageBase* msg);

// Verify authentication information in melon_std format
bool VerifyMStdRequest(const InputMessageBase* msg);

// Pack `request' to `method' into `buf'.
void PackMStdRequest(mutil::IOBuf* buf,
                    SocketMessage**,
                    uint64_t correlation_id,
                    const google::protobuf::MethodDescriptor* method,
                    Controller* controller,
                    const mutil::IOBuf& request,
                    const Authenticator* auth);

}  // namespace policy
} // namespace melon
