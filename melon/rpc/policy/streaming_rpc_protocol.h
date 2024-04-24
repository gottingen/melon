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
#include "melon/proto/rpc/streaming_rpc_meta.pb.h"


namespace melon {
namespace policy {

void PackStreamMessage(mutil::IOBuf* out,
                       const StreamFrameMeta &fm,
                       const mutil::IOBuf *data);

ParseResult ParseStreamingMessage(mutil::IOBuf* source, Socket* socket,
                                  bool read_eof, const void* arg);

void ProcessStreamingMessage(InputMessageBase* msg);

void SendStreamRst(Socket* sock, int64_t remote_stream_id);

void SendStreamClose(Socket *sock, int64_t remote_stream_id,
                     int64_t source_stream_id);

int SendStreamData(Socket* sock, const mutil::IOBuf* data,
                   int64_t remote_stream_id, int64_t source_stream_id);

}  // namespace policy
} // namespace melon

