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



#include <melon/rpc/policy/streaming_rpc_protocol.h>
#include <cinttypes>
#include <google/protobuf/descriptor.h>         // MethodDescriptor
#include <google/protobuf/message.h>            // Message
#include <gflags/gflags.h>
#include <melon/base/macros.h>
#include <turbo/log/logging.h>                       // LOG()
#include <melon/utility/time.h>
#include <melon/base/iobuf.h>                         // mutil::IOBuf
#include <melon/base/raw_pack.h>                      // RawPacker RawUnpacker
#include <melon/rpc/log.h>
#include <melon/rpc/socket.h>                        // Socket
#include <melon/proto/rpc/streaming_rpc_meta.pb.h>         // StreamFrameMeta
#include <melon/rpc/policy/most_common_message.h>
#include <melon/rpc/stream_impl.h>


namespace melon {
namespace policy {

// Notes on Streaming RPC Protocol:
// 1 - Header format is [STRM][body_size][meta_size], 12 bytes in total
// 2 - body_size and meta_size are in network byte order
void PackStreamMessage(mutil::IOBuf* out,
                       const StreamFrameMeta &fm,
                       const mutil::IOBuf *data) {
    const uint32_t data_length = data ? data->length() : 0;
    const uint32_t meta_length = GetProtobufByteSize(fm);
    char head[12];
    uint32_t* dummy = (uint32_t*)head;  // suppresses strict-alias warning
    *(uint32_t*)dummy = *(const uint32_t*)"STRM";
    mutil::RawPacker(head + 4)
        .pack32(data_length + meta_length)
        .pack32(meta_length);
    out->append(head, ARRAY_SIZE(head));
    mutil::IOBufAsZeroCopyOutputStream wrapper(out);
    CHECK(fm.SerializeToZeroCopyStream(&wrapper));
    if (data != NULL) {
        out->append(*data);
    }
}

ParseResult ParseStreamingMessage(mutil::IOBuf* source,
                            Socket* socket, bool /*read_eof*/, const void* /*arg*/) {
    char header_buf[12];
    const size_t n = source->copy_to(header_buf, sizeof(header_buf));
    if (n >= 4) {
        void* dummy = header_buf;
        if (*(const uint32_t*)dummy != *(const uint32_t*)"STRM") {
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
    mutil::RawUnpacker(header_buf + 4).unpack32(body_size).unpack32(meta_size);
    if (body_size > FLAGS_max_body_size) {
        return MakeParseError(PARSE_ERROR_TOO_BIG_DATA);
    } else if (source->length() < sizeof(header_buf) + body_size) {
        return MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA);
    }
    if (MELON_UNLIKELY(meta_size > body_size)) {
        LOG(ERROR) << "meta_size=" << meta_size << " is bigger than body_size="
                   << body_size;
        // Pop the message
        source->pop_front(sizeof(header_buf) + body_size);
        return MakeParseError(PARSE_ERROR_TRY_OTHERS);
    }
    source->pop_front(sizeof(header_buf));
    mutil::IOBuf meta_buf;
    source->cutn(&meta_buf, meta_size);
    mutil::IOBuf payload;
    source->cutn(&payload, body_size - meta_size);

    do {
        StreamFrameMeta fm;
        if (!ParsePbFromIOBuf(&fm, meta_buf)) {
            LOG(WARNING) << "Fail to Parse StreamFrameMeta from " << *socket;
            break;
        }
        SocketUniquePtr ptr;
        if (Socket::Address((SocketId)fm.stream_id(), &ptr) != 0) {
            RPC_VLOG_IF(fm.frame_type() != FRAME_TYPE_RST
                            && fm.frame_type() != FRAME_TYPE_CLOSE
                            && fm.frame_type() != FRAME_TYPE_FEEDBACK)
                   << "Fail to find stream=" << fm.stream_id();
            // It's normal that the stream is closed before receiving feedback frames from peer.
            // In this case, RST frame should not be sent to peer, otherwise on-fly data can be lost.
            if (fm.has_source_stream_id() && fm.frame_type() != FRAME_TYPE_FEEDBACK) {
                SendStreamRst(socket, fm.source_stream_id());
            }
            break;
        }
        meta_buf.clear();  // to reduce memory resident
        ((Stream*)ptr->conn())->OnReceived(fm, &payload, socket);
    } while (0);

    // Hack input messenger
    return MakeMessage(NULL);
}

void ProcessStreamingMessage(InputMessageBase* /*msg*/) {
    CHECK(false) << "Should never be called";
}

void SendStreamRst(Socket *sock, int64_t remote_stream_id) {
    CHECK(sock != NULL);
    StreamFrameMeta fm;
    fm.set_stream_id(remote_stream_id);
    fm.set_frame_type(FRAME_TYPE_RST);
    mutil::IOBuf out;
    PackStreamMessage(&out, fm, NULL);
    sock->Write(&out);
}

void SendStreamClose(Socket *sock, int64_t remote_stream_id,
                     int64_t source_stream_id) {
    CHECK(sock != NULL);
    StreamFrameMeta fm;
    fm.set_stream_id(remote_stream_id);
    fm.set_source_stream_id(source_stream_id);
    fm.set_frame_type(FRAME_TYPE_CLOSE);
    mutil::IOBuf out;
    PackStreamMessage(&out, fm, NULL);
    sock->Write(&out);
}

int SendStreamData(Socket* sock, const mutil::IOBuf* data,
                   int64_t remote_stream_id, int64_t source_stream_id) {
    StreamFrameMeta fm;
    fm.set_stream_id(remote_stream_id);
    fm.set_source_stream_id(source_stream_id);
    fm.set_frame_type(FRAME_TYPE_DATA);
    fm.set_has_continuation(false);
    mutil::IOBuf out;
    PackStreamMessage(&out, fm, data);
    return sock->Write(&out);
}

}  // namespace policy
} // namespace melon
