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


#ifndef BRPC_POLICY_ESP_PROTOCOL_H
#define BRPC_POLICY_ESP_PROTOCOL_H

#include <unistd.h>
#include <sys/types.h>

#include "melon/rpc/protocol.h"


namespace melon {
namespace policy {

ParseResult ParseEspMessage(
        mutil::IOBuf* source,
        Socket* socket, 
        bool read_eof, 
        const void *arg);

void SerializeEspRequest(
        mutil::IOBuf* request_buf,
        Controller* controller,
        const google::protobuf::Message* request);

void PackEspRequest(mutil::IOBuf* packet_buf,
                    SocketMessage**,
                    uint64_t correlation_id,
                    const google::protobuf::MethodDescriptor*,
                    Controller* controller,
                    const mutil::IOBuf&,
                    const Authenticator*);

void ProcessEspResponse(InputMessageBase* msg);

} // namespace policy
} // namespace melon


#endif // BRPC_POLICY_ESP_PROTOCOL_H
