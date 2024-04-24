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



#include <google/protobuf/descriptor.h>         // MethodDescriptor
#include <google/protobuf/message.h>            // Message
#include <gflags/gflags.h>
#include "melon/utility/logging.h"                       // LOG()
#include "melon/utility/time.h"
#include "melon/utility/iobuf.h"                         // mutil::IOBuf
#include "melon/utility/sys_byteorder.h"
#include "melon/rpc/controller.h"               // Controller
#include "melon/rpc/details/controller_private_accessor.h"
#include "melon/rpc/socket.h"                   // Socket
#include "melon/rpc/server.h"                   // Server
#include "melon/rpc/details/server_private_accessor.h"
#include "melon/rpc/span.h"
#include "melon/rpc/compress.h"                 // ParseFromCompressedData
#include "melon/rpc/policy/memcache_binary_protocol.h"
#include "melon/rpc/policy/memcache_binary_header.h"
#include "melon/rpc/memcache/memcache.h"
#include "melon/rpc/policy/most_common_message.h"
#include "melon/utility/containers/flat_map.h"


namespace melon {

DECLARE_bool(enable_rpcz);

namespace policy {

MELON_CASSERT(sizeof(MemcacheRequestHeader) == 24, must_match);
MELON_CASSERT(sizeof(MemcacheResponseHeader) == 24, must_match);

static uint64_t supported_cmd_map[8];
static pthread_once_t supported_cmd_map_once = PTHREAD_ONCE_INIT;

static void InitSupportedCommandMap() {
    mutil::bit_array_clear(supported_cmd_map, 256);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_GET);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_SET);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_ADD);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_REPLACE);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_DELETE);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_INCREMENT);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_DECREMENT);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_FLUSH);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_VERSION);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_NOOP);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_APPEND);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_PREPEND);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_STAT);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_TOUCH);
    mutil::bit_array_set(supported_cmd_map, MC_BINARY_SASL_AUTH);
}

inline bool IsSupportedCommand(uint8_t command) {
    pthread_once(&supported_cmd_map_once, InitSupportedCommandMap);
    return mutil::bit_array_get(supported_cmd_map, command);
}

ParseResult ParseMemcacheMessage(mutil::IOBuf* source,
                                 Socket* socket, bool /*read_eof*/, const void */*arg*/) {
    while (1) {
        const uint8_t* p_mcmagic = (const uint8_t*)source->fetch1();
        if (NULL == p_mcmagic) {
            return MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA);
        }            
        if (*p_mcmagic != (uint8_t)MC_MAGIC_RESPONSE) {
            return MakeParseError(PARSE_ERROR_TRY_OTHERS);
        }
        char buf[24];
        const uint8_t* p = (const uint8_t*)source->fetch(buf, sizeof(buf));
        if (NULL == p) {
            return MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA);
        }
        const MemcacheResponseHeader* header = (const MemcacheResponseHeader*)p;
        uint32_t total_body_length = mutil::NetToHost32(header->total_body_length);
        if (source->size() < sizeof(*header) + total_body_length) {
            return MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA);
        }

        if (!IsSupportedCommand(header->command)) {
            MLOG(WARNING) << "Not support command=" << header->command;
            source->pop_front(sizeof(*header) + total_body_length);
            return MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA);
        }
        
        PipelinedInfo pi;
        if (!socket->PopPipelinedInfo(&pi)) {
            MLOG(WARNING) << "No corresponding PipelinedInfo in socket, drop";
            source->pop_front(sizeof(*header) + total_body_length);
            return MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA);
        }
        MostCommonMessage* msg =
            static_cast<MostCommonMessage*>(socket->parsing_context());
        if (msg == NULL) {
            msg = MostCommonMessage::Get();
            socket->reset_parsing_context(msg);
        }

        // endianness conversions.
        const MemcacheResponseHeader local_header = {
            header->magic,
            header->command,
            mutil::NetToHost16(header->key_length),
            header->extras_length,
            header->data_type,
            mutil::NetToHost16(header->status),
            total_body_length,
            mutil::NetToHost32(header->opaque),
            mutil::NetToHost64(header->cas_value),
        };
        msg->meta.append(&local_header, sizeof(local_header));
        source->pop_front(sizeof(*header));
        source->cutn(&msg->meta, total_body_length);
        if (header->command == MC_BINARY_SASL_AUTH) {
            if (header->status != 0) {
                MLOG(ERROR) << "Failed to authenticate the couchbase bucket.";
                return MakeParseError(PARSE_ERROR_NO_RESOURCE, 
                                      "Fail to authenticate with the couchbase bucket");
            }
            DestroyingPtr<MostCommonMessage> auth_msg(
                 static_cast<MostCommonMessage*>(socket->release_parsing_context()));
            socket->GivebackPipelinedInfo(pi);
        } else {
            if (++msg->pi.count >= pi.count) {
                MCHECK_EQ(msg->pi.count, pi.count);
                msg = static_cast<MostCommonMessage*>(socket->release_parsing_context());
                msg->pi = pi;
                return MakeMessage(msg);
            } else {
                socket->GivebackPipelinedInfo(pi);
            }
        }    
    }
}

void ProcessMemcacheResponse(InputMessageBase* msg_base) {
    const int64_t start_parse_us = mutil::cpuwide_time_us();
    DestroyingPtr<MostCommonMessage> msg(static_cast<MostCommonMessage*>(msg_base));

    const fiber_session_t cid = msg->pi.id_wait;
    Controller* cntl = NULL;
    const int rc = fiber_session_lock(cid, (void**)&cntl);
    if (rc != 0) {
        LOG_IF(ERROR, rc != EINVAL && rc != EPERM)
            << "Fail to lock correlation_id=" << cid << ": " << berror(rc);
        return;
    }
    
    ControllerPrivateAccessor accessor(cntl);
    Span* span = accessor.span();
    if (span) {
        span->set_base_real_us(msg->base_real_us());
        span->set_received_us(msg->received_us());
        span->set_response_size(msg->meta.length());
        span->set_start_parse_us(start_parse_us);
    }
    const int saved_error = cntl->ErrorCode();
    if (cntl->response() == NULL) {
        cntl->SetFailed(ERESPONSE, "response is NULL!");
    } else if (cntl->response()->GetDescriptor() != MemcacheResponse::descriptor()) {
        cntl->SetFailed(ERESPONSE, "Must be MemcacheResponse");
    } else {
        // We work around ParseFrom of pb which is just a placeholder.
        ((MemcacheResponse*)cntl->response())->raw_buffer() = msg->meta.movable();
        if (msg->pi.count != accessor.pipelined_count()) {
            cntl->SetFailed(ERESPONSE, "pipelined_count=%d of response does "
                                  "not equal request's=%d",
                                  msg->pi.count, accessor.pipelined_count());
        }
    }
    // Unlocks correlation_id inside. Revert controller's
    // error code if it version check of `cid' fails
    msg.reset();  // optional, just release resource ASAP
    accessor.OnResponse(cid, saved_error);
}

void SerializeMemcacheRequest(mutil::IOBuf* buf,
                              Controller* cntl,
                              const google::protobuf::Message* request) {
    if (request == NULL) {
        return cntl->SetFailed(EREQUEST, "request is NULL");
    }
    if (request->GetDescriptor() != MemcacheRequest::descriptor()) {
        return cntl->SetFailed(EREQUEST, "Must be MemcacheRequest");
    }
    const MemcacheRequest* mr = (const MemcacheRequest*)request;
    // We work around SerializeTo of pb which is just a placeholder.
    *buf = mr->raw_buffer();
    ControllerPrivateAccessor(cntl).set_pipelined_count(mr->pipelined_count());
}

void PackMemcacheRequest(mutil::IOBuf* buf,
                         SocketMessage**,
                         uint64_t /*correlation_id*/,
                         const google::protobuf::MethodDescriptor*,
                         Controller* cntl,
                         const mutil::IOBuf& request,
                         const Authenticator* auth) {
    if (auth) {
        std::string auth_str;
        if (auth->GenerateCredential(&auth_str) != 0) {
            return cntl->SetFailed(EREQUEST, "Fail to generate credential");
        }
        buf->append(auth_str);
    }
    buf->append(request);
}

const std::string& GetMemcacheMethodName(
    const google::protobuf::MethodDescriptor*,
    const Controller*) {
    const static std::string MEMCACHED_STR = "memcached";
    return MEMCACHED_STR;
}

}  // namespace policy
} // namespace melon
