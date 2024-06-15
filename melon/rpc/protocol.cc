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



// Since kDefaultTotalBytesLimit is private, we need some hacks to get the limit.
// Works for pb 2.4, 2.6, 3.0
#define private public
#include <google/protobuf/io/coded_stream.h>
const int PB_TOTAL_BYETS_LIMITS_RAW =
    google::protobuf::io::CodedInputStream::kDefaultTotalBytesLimit;
const uint64_t PB_TOTAL_BYETS_LIMITS =
    PB_TOTAL_BYETS_LIMITS_RAW < 0 ? (uint64_t)-1LL : PB_TOTAL_BYETS_LIMITS_RAW;
#undef private

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>
#include <gflags/gflags.h>
#include <turbo/log/logging.h>
#include <melon/utility/memory/singleton_on_pthread_once.h>
#include <melon/rpc/protocol.h>
#include <melon/rpc/controller.h>
#include <melon/rpc/compress.h>
#include <melon/rpc/global.h>
#include <melon/rpc/serialized_request.h>
#include <melon/rpc/input_messenger.h>


namespace melon {

DEFINE_uint64(max_body_size, 64 * 1024 * 1024,
              "Maximum size of a single message body in all protocols");

DEFINE_bool(log_error_text, false,
            "Print Controller.ErrorText() when server is about to"
            " respond a failed RPC");
MELON_VALIDATE_GFLAG(log_error_text, PassValidate);

// Not using ProtocolType_MAX as the boundary because others may define new
// protocols outside melon.
const size_t MAX_PROTOCOL_SIZE = 128;
struct ProtocolEntry {
    std::atomic<bool> valid;
    Protocol protocol;
    
    ProtocolEntry() : valid(false) {}
};
struct ProtocolMap {
    ProtocolEntry entries[MAX_PROTOCOL_SIZE];
};
inline ProtocolEntry* get_protocol_map() {
    return mutil::get_leaky_singleton<ProtocolMap>()->entries;
}
static pthread_mutex_t s_protocol_map_mutex = PTHREAD_MUTEX_INITIALIZER;

int RegisterProtocol(ProtocolType type, const Protocol& protocol) {
    const size_t index = type;
    if (index >= MAX_PROTOCOL_SIZE) {
        LOG(ERROR) << "ProtocolType=" << type << " is out of range";
        return -1;
    }
    if (!protocol.support_client() && !protocol.support_server()) {
        LOG(ERROR) << "ProtocolType=" << type
                   << " neither supports client nor server";
        return -1;
    }
    ProtocolEntry* const protocol_map = get_protocol_map();
    MELON_SCOPED_LOCK(s_protocol_map_mutex);
    if (protocol_map[index].valid.load(std::memory_order_relaxed)) {
        LOG(ERROR) << "ProtocolType=" << type << " was registered";
        return -1;
    }
    protocol_map[index].protocol = protocol;
    protocol_map[index].valid.store(true, std::memory_order_release);
    return 0;
}

// Called frequently, must be fast.
const Protocol* FindProtocol(ProtocolType type) {
    const size_t index = type;
    if (index >= MAX_PROTOCOL_SIZE) {
        LOG(ERROR) << "ProtocolType=" << type << " is out of range";
        return NULL;
    }
    ProtocolEntry* const protocol_map = get_protocol_map();
    if (protocol_map[index].valid.load(std::memory_order_acquire)) {
        return &protocol_map[index].protocol;
    }
    return NULL;
}

void ListProtocols(std::vector<Protocol>* vec) {
    vec->clear();
    ProtocolEntry* const protocol_map = get_protocol_map();
    for (size_t i = 0; i < MAX_PROTOCOL_SIZE; ++i) {
        if (protocol_map[i].valid.load(std::memory_order_acquire)) {
            vec->push_back(protocol_map[i].protocol);
        }
    }
}

void ListProtocols(std::vector<std::pair<ProtocolType, Protocol> >* vec) {
    vec->clear();
    ProtocolEntry* const protocol_map = get_protocol_map();
    for (size_t i = 0; i < MAX_PROTOCOL_SIZE; ++i) {
        if (protocol_map[i].valid.load(std::memory_order_acquire)) {
            vec->emplace_back((ProtocolType)i, protocol_map[i].protocol);
        }
    }
}

void SerializeRequestDefault(mutil::IOBuf* buf,
                             Controller* cntl,
                             const google::protobuf::Message* request) {
    // Check sanity of request.
    if (!request) {
        return cntl->SetFailed(EREQUEST, "`request' is NULL");
    }
    if (request->GetDescriptor() == SerializedRequest::descriptor()) {
        buf->append(((SerializedRequest*)request)->serialized_data());
        return;
    }
    if (!request->IsInitialized()) {
        return cntl->SetFailed(
            EREQUEST, "Missing required fields in request: %s",
            request->InitializationErrorString().c_str());
    }
    if (!SerializeAsCompressedData(*request, buf, cntl->request_compress_type())) {
        return cntl->SetFailed(
            EREQUEST, "Fail to compress request, compress_type=%d",
            (int)cntl->request_compress_type());
    }
}

// ======================================================

inline bool CompareStringPieceWithoutCase(
        const mutil::StringPiece& s1, const char* s2) {
    if (strlen(s2) != s1.size()) {
        return false;
    }
    return strncasecmp(s1.data(), s2, s1.size()) == 0;
}

ProtocolType StringToProtocolType(const mutil::StringPiece& name,
                                  bool print_log_on_unknown) {
    // Force init of s_protocol_name.
    GlobalInitializeOrDie();

    ProtocolEntry* const protocol_map = get_protocol_map();
    for (size_t i = 0; i < MAX_PROTOCOL_SIZE; ++i) {
        if (protocol_map[i].valid.load(std::memory_order_acquire) &&
            CompareStringPieceWithoutCase(name, protocol_map[i].protocol.name)) {
            return static_cast<ProtocolType>(i);
        }
    }
    // We need to print a log here otherwise the return value cannot reflect
    // the original input, which makes later initializations of other classes
    // fail with vague logs which is not informational to user, like this:
    //   "channel doesn't support protocol=unknown"
    // Some callsite may not need this log, so we keep a flag.
    if (print_log_on_unknown) {
        std::ostringstream err;
        err << "Unknown protocol `" << name << "', supported protocols:";
        for (size_t i = 0; i < MAX_PROTOCOL_SIZE; ++i) {
            if (protocol_map[i].valid.load(std::memory_order_acquire)) {
                err << ' ' << protocol_map[i].protocol.name;
            }
        }
        LOG(ERROR) << err.str();
    }
    return PROTOCOL_UNKNOWN;
}

const char* ProtocolTypeToString(ProtocolType type) {
    // Force init of s_protocol_name.
    GlobalInitializeOrDie();
    
    const Protocol* p = FindProtocol(type);
    if (p != NULL) {
        return p->name;
    }
    return "unknown";
}

MUTIL_FORCE_INLINE bool ParsePbFromZeroCopyStreamInlined(
    google::protobuf::Message* msg,
    google::protobuf::io::ZeroCopyInputStream* input) {
    google::protobuf::io::CodedInputStream decoder(input);
    // Remove the limit inside pb so that it never conflicts with -max_body_size 
    // According to source code of pb, SetTotalBytesLimit is not a simple set,
    // avoid calling the function when the limit is definitely unreached.
    if (PB_TOTAL_BYETS_LIMITS < FLAGS_max_body_size) {
#if GOOGLE_PROTOBUF_VERSION >= 3006000
        decoder.SetTotalBytesLimit(INT_MAX);
#else
        decoder.SetTotalBytesLimit(INT_MAX, -1);
#endif
    }
    return msg->ParseFromCodedStream(&decoder) && decoder.ConsumedEntireMessage();
}

MUTIL_FORCE_INLINE bool ParsePbTextFromZeroCopyStreamInlined(
    google::protobuf::Message* msg,
    google::protobuf::io::ZeroCopyInputStream* input) {
    return google::protobuf::TextFormat::Parse(input, msg);
}

bool ParsePbFromZeroCopyStream(
    google::protobuf::Message* msg,
    google::protobuf::io::ZeroCopyInputStream* input) {
    return ParsePbFromZeroCopyStreamInlined(msg, input);
}

bool ParsePbTextFromIOBuf(google::protobuf::Message* msg, const mutil::IOBuf& buf) {
    mutil::IOBufAsZeroCopyInputStream stream(buf);
    return ParsePbTextFromZeroCopyStreamInlined(msg, &stream);
}

bool ParsePbFromIOBuf(google::protobuf::Message* msg, const mutil::IOBuf& buf) {
    mutil::IOBufAsZeroCopyInputStream stream(buf);
    return ParsePbFromZeroCopyStreamInlined(msg, &stream);
}

bool ParsePbFromArray(google::protobuf::Message* msg,
                      const void* data, size_t size) {
    google::protobuf::io::ArrayInputStream stream(data, size);
    return ParsePbFromZeroCopyStreamInlined(msg, &stream);
}

bool ParsePbFromString(google::protobuf::Message* msg, const std::string& str) {
    google::protobuf::io::ArrayInputStream stream(str.data(), str.size());
    return ParsePbFromZeroCopyStreamInlined(msg, &stream);
}

void LogErrorTextAndDelete::operator()(Controller* c) const {
    if (!c) {
        return;
    }
    if (FLAGS_log_error_text && c->ErrorCode()) {
        if (c->ErrorCode() == ECLOSE) {
            LOG(WARNING) << "Close connection to " << c->remote_side()
                         << ": " << c->ErrorText();
        } else {
            LOG(WARNING) << "Error to " << c->remote_side()
                         << ": " << c->ErrorText();
        }
    }
    if (_delete_cntl) {
        delete c;
    }
}

} // namespace melon
