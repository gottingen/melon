// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.


#ifndef  MELON_RPC_STREAMING_RPC_PROTOCOL_H_
#define  MELON_RPC_STREAMING_RPC_PROTOCOL_H_

#include "melon/rpc/protocol.h"
#include "melon/rpc/streaming_rpc_meta.pb.h"


namespace melon::rpc {
    namespace policy {

        void PackStreamMessage(melon::cord_buf *out,
                               const StreamFrameMeta &fm,
                               const melon::cord_buf *data);

        ParseResult ParseStreamingMessage(melon::cord_buf *source, Socket *socket,
                                          bool read_eof, const void *arg);

        void ProcessStreamingMessage(InputMessageBase *msg);

        void SendStreamRst(Socket *sock, int64_t remote_stream_id);

        void SendStreamClose(Socket *sock, int64_t remote_stream_id,
                             int64_t source_stream_id);

        int SendStreamData(Socket *sock, const melon::cord_buf *data,
                           int64_t remote_stream_id, int64_t source_stream_id);

    }  // namespace policy
} // namespace melon::rpc


#endif  // MELON_RPC_STREAMING_RPC_PROTOCOL_H_
