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


#include "melon/rpc/policy/streaming_rpc_protocol.h"

#include <google/protobuf/descriptor.h>         // MethodDescriptor
#include <google/protobuf/message.h>            // Message
#include <gflags/gflags.h>
#include "melon/base/profile.h"
#include "melon/log/logging.h"                       // MELON_LOG()
#include "melon/times/time.h"
#include "melon/io/cord_buf.h"                         // melon::cord_buf
#include "melon/io/raw_pack.h"                      // raw_packer raw_unpacker
#include "melon/rpc/log.h"
#include "melon/rpc/socket.h"                        // Socket
#include "melon/rpc/streaming_rpc_meta.pb.h"         // StreamFrameMeta
#include "melon/rpc/policy/most_common_message.h"
#include "melon/rpc/stream_impl.h"


namespace melon::rpc {
    namespace policy {

        // Notes on Streaming RPC Protocol:
        // 1 - Header format is [STRM][body_size][meta_size], 12 bytes in total
        // 2 - body_size and meta_size are in network byte order
        void PackStreamMessage(melon::cord_buf *out,
                               const StreamFrameMeta &fm,
                               const melon::cord_buf *data) {
            const uint32_t data_length = data ? data->length() : 0;
            const uint32_t meta_length = fm.ByteSizeLong();
            char head[12];
            uint32_t *dummy = (uint32_t *) head;  // suppresses strict-alias warning
            *(uint32_t *) dummy = *(const uint32_t *) "STRM";
            melon::raw_packer(head + 4)
                    .pack32(data_length + meta_length)
                    .pack32(meta_length);
            out->append(head, MELON_ARRAY_SIZE(head));
            melon::cord_buf_as_zero_copy_output_stream wrapper(out);
            MELON_CHECK(fm.SerializeToZeroCopyStream(&wrapper));
            if (data != NULL) {
                out->append(*data);
            }
        }

        ParseResult ParseStreamingMessage(melon::cord_buf *source,
                                          Socket *socket, bool /*read_eof*/, const void * /*arg*/) {
            char header_buf[12];
            const size_t n = source->copy_to(header_buf, sizeof(header_buf));
            if (n >= 4) {
                void *dummy = header_buf;
                if (*(const uint32_t *) dummy != *(const uint32_t *) "STRM") {
                    return MakeParseError(PARSE_ERROR_TRY_OTHERS);
                }
            } else {
                if (memcmp(header_buf, "STRM", n) != 0) {
                    return MakeParseError(PARSE_ERROR_TRY_OTHERS);
                }
            }
            if (n < sizeof(header_buf)) {
                return MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA);
            }
            uint32_t body_size;
            uint32_t meta_size;
            melon::raw_unpacker(header_buf + 4).unpack32(body_size).unpack32(meta_size);
            if (body_size > FLAGS_max_body_size) {
                return MakeParseError(PARSE_ERROR_TOO_BIG_DATA);
            } else if (source->length() < sizeof(header_buf) + body_size) {
                return MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA);
            }
            if (MELON_UNLIKELY(meta_size > body_size)) {
                MELON_LOG(ERROR) << "meta_size=" << meta_size << " is bigger than body_size="
                                 << body_size;
                // Pop the message
                source->pop_front(sizeof(header_buf) + body_size);
                return MakeParseError(PARSE_ERROR_TRY_OTHERS);
            }
            source->pop_front(sizeof(header_buf));
            melon::cord_buf meta_buf;
            source->cutn(&meta_buf, meta_size);
            melon::cord_buf payload;
            source->cutn(&payload, body_size - meta_size);

            do {
                StreamFrameMeta fm;
                if (!ParsePbFromCordBuf(&fm, meta_buf)) {
                    MELON_LOG(WARNING) << "Fail to Parse StreamFrameMeta from " << *socket;
                    break;
                }
                SocketUniquePtr ptr;
                if (Socket::Address((SocketId) fm.stream_id(), &ptr) != 0) {
                    RPC_VLOG_IF(fm.frame_type() != FRAME_TYPE_RST
                                && fm.frame_type() != FRAME_TYPE_CLOSE
                                && fm.frame_type() != FRAME_TYPE_FEEDBACK)
                                    << "Fail to find stream=" << fm.stream_id();
                    if (fm.has_source_stream_id()) {
                        SendStreamRst(socket, fm.source_stream_id());
                    }
                    break;
                }
                meta_buf.clear();  // to reduce memory resident
                ((Stream *) ptr->conn())->OnReceived(fm, &payload, socket);
            } while (0);

            // Hack input messenger
            return MakeMessage(NULL);
        }

        void ProcessStreamingMessage(InputMessageBase * /*msg*/) {
            MELON_CHECK(false) << "Should never be called";
        }

        void SendStreamRst(Socket *sock, int64_t remote_stream_id) {
            MELON_CHECK(sock != NULL);
            StreamFrameMeta fm;
            fm.set_stream_id(remote_stream_id);
            fm.set_frame_type(FRAME_TYPE_RST);
            melon::cord_buf out;
            PackStreamMessage(&out, fm, NULL);
            sock->Write(&out);
        }

        void SendStreamClose(Socket *sock, int64_t remote_stream_id,
                             int64_t source_stream_id) {
            MELON_CHECK(sock != NULL);
            StreamFrameMeta fm;
            fm.set_stream_id(remote_stream_id);
            fm.set_source_stream_id(source_stream_id);
            fm.set_frame_type(FRAME_TYPE_CLOSE);
            melon::cord_buf out;
            PackStreamMessage(&out, fm, NULL);
            sock->Write(&out);
        }

        int SendStreamData(Socket *sock, const melon::cord_buf *data,
                           int64_t remote_stream_id, int64_t source_stream_id) {
            StreamFrameMeta fm;
            fm.set_stream_id(remote_stream_id);
            fm.set_source_stream_id(source_stream_id);
            fm.set_frame_type(FRAME_TYPE_DATA);
            fm.set_has_continuation(false);
            melon::cord_buf out;
            PackStreamMessage(&out, fm, data);
            return sock->Write(&out);
        }

    }  // namespace policy
} // namespace melon::rpc
