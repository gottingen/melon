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

#include <vector>                                  // std::vector
#include <stdint.h>                                // uint64_t
#include <gflags/gflags_declare.h>                 // DECLARE_xxx
#include <melon/base/endpoint.h>                         // mutil::EndPoint
#include <melon/base/iobuf.h>
#include <turbo/log/logging.h>
#include <melon/proto/rpc/options.pb.h>                  // ProtocolType
#include <melon/rpc/socket_id.h>                   // SocketId
#include <melon/rpc/parse_result.h>                // ParseResult
#include <melon/rpc/adaptive_connection_type.h>
#include <melon/rpc/adaptive_protocol_type.h>

namespace google {
    namespace protobuf {
        class Message;

        class MethodDescriptor;
    }  // namespace protobuf
}  // namespace google

namespace mutil {
    class IOBuf;
}


namespace melon {
    class Socket;

    class SocketMessage;

    class Controller;

    class Authenticator;

    class InputMessageBase;

    DECLARE_uint64(max_body_size);
    DECLARE_bool(log_error_text);

// Get the serialized byte size of the protobuf message, 
// different versions of protobuf have different methods
// use template to avoid include `google/protobuf/message.h`
    template<typename T>
    inline uint32_t GetProtobufByteSize(const T &message) {
#if GOOGLE_PROTOBUF_VERSION >= 3010000
        return message.ByteSizeLong();
#else
        return static_cast<uint32_t>(message.ByteSize());
#endif
    }

// 3 steps to add a new Protocol:
// Step1: Add a new ProtocolType in src/melon/options.proto
//        as identifier of the Protocol.
// Step2: Implement callbacks of struct `Protocol' in policy/ directory.
// Step3: Register the protocol in global.cpp using `RegisterProtocol'

    struct Protocol {
        // [Required by both client and server]
        // The callback to cut a message from `source'.
        // Returned message will be passed to process_request and process_response
        // later and Destroy()-ed by InputMessenger.
        // Returns:
        //   MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA):
        //     `source' does not form a complete message yet.
        //   MakeParseError(PARSE_ERROR_TRY_OTHERS).
        //     `source' does not fit the protocol, the data should be tried by
        //     other protocols. If the data is definitely corrupted (e.g. magic
        //     header matches but other fields are wrong), pop corrupted part
        //     from `source' before returning.
        //  MakeMessage(InputMessageBase*):
        //     The message is parsed successfully and cut from `source'.
        typedef ParseResult (*Parse)(mutil::IOBuf *source, Socket *socket,
                                     bool read_eof, const void *arg);

        Parse parse;

        // [Required by client]
        // The callback to serialize `request' into `request_buf' which will be
        // packed into message by pack_request later. Called once for each RPC.
        // `cntl' provides additional data needed by some protocol (say HTTP).
        // Call cntl->SetFailed() on error.
        typedef void (*SerializeRequest)(
                mutil::IOBuf *request_buf,
                Controller *cntl,
                const google::protobuf::Message *request);

        SerializeRequest serialize_request;

        // [Required by client]
        // The callback to pack `request_buf' into `iobuf_out' or `user_message_out'
        // Called before sending each request (including retries).
        // Remember to pack authentication information when `auth' is not NULL.
        // Call cntl->SetFailed() on error.
        typedef void (*PackRequest)(
                mutil::IOBuf *iobuf_out,
                SocketMessage **user_message_out,
                uint64_t correlation_id,
                const google::protobuf::MethodDescriptor *method,
                Controller *controller,
                const mutil::IOBuf &request_buf,
                const Authenticator *auth);

        PackRequest pack_request;

        // [Required by server]
        // The callback to handle request `msg' created by a successful parse().
        // `msg' must be Destroy()-ed when the processing is done. To make sure
        // Destroy() is always called, consider using DestroyingPtr<> defined in
        // destroyable.h
        // May be called in a different thread from parse().
        typedef void (*ProcessRequest)(InputMessageBase *msg);

        ProcessRequest process_request;

        // [Required by client]
        // The callback to handle response `msg' created by a successful parse().
        // `msg' must be Destroy()-ed when the processing is done. To make sure
        // Destroy() is always called, consider using DestroyingPtr<> defined in
        // destroyable.h
        // May be called in a different thread from parse().
        typedef void (*ProcessResponse)(InputMessageBase *msg);

        ProcessResponse process_response;

        // [Required by authenticating server]
        // The callback to verify authentication of this socket. Only called
        // on the first message that a socket receives. Can be NULL when
        // authentication is not needed or this is the client side.
        // Returns true on successful authentication.
        typedef bool (*Verify)(const InputMessageBase *msg);

        Verify verify;

        // [Optional]
        // Convert `server_addr_and_port'(a parameter to Channel) to mutil::EndPoint.
        typedef bool (*ParseServerAddress)(mutil::EndPoint *out,
                                           const char *server_addr_and_port);

        ParseServerAddress parse_server_address;

        // [Optional] Customize method name.
        typedef const std::string &(*GetMethodName)(
                const google::protobuf::MethodDescriptor *method,
                const Controller *);

        GetMethodName get_method_name;

        // Bitwise-or of supported ConnectionType
        ConnectionType supported_connection_type;

        // Name of this protocol, must be string constant.
        const char *name;

        // True if this protocol is supported at client-side.
        bool support_client() const {
            return serialize_request && pack_request && process_response;
        }

        // True if this protocol is supported at server-side.
        bool support_server() const { return process_request; }
    };

    const ConnectionType CONNECTION_TYPE_POOLED_AND_SHORT =
            (ConnectionType) ((int) CONNECTION_TYPE_POOLED |
                              (int) CONNECTION_TYPE_SHORT);

    const ConnectionType CONNECTION_TYPE_ALL =
            (ConnectionType) ((int) CONNECTION_TYPE_SINGLE |
                              (int) CONNECTION_TYPE_POOLED |
                              (int) CONNECTION_TYPE_SHORT);

    // [thread-safe]
    // Register `protocol' using key=`type'.
    // Returns 0 on success, -1 otherwise
    int RegisterProtocol(ProtocolType type, const Protocol &protocol);

    // [thread-safe]
    // Find the protocol registered with key=`type'.
    // Returns NULL on not found.
    const Protocol *FindProtocol(ProtocolType type);

    // [thread-safe]
    // List all registered protocols into `vec'.
    void ListProtocols(std::vector<Protocol> *vec);

    void ListProtocols(std::vector<std::pair<ProtocolType, Protocol> > *vec);

    // The common serialize_request implementation used by many protocols.
    void SerializeRequestDefault(mutil::IOBuf *buf,
                                 Controller *cntl,
                                 const google::protobuf::Message *request);

    // Replacements for msg->ParseFromXXX() to make the bytes limit in pb
    // consistent with -max_body_size
    bool ParsePbFromZeroCopyStream(google::protobuf::Message *msg,
                                   google::protobuf::io::ZeroCopyInputStream *input);

    bool ParsePbFromIOBuf(google::protobuf::Message *msg, const mutil::IOBuf &buf);

    bool ParsePbTextFromIOBuf(google::protobuf::Message *msg, const mutil::IOBuf &buf);

    bool ParsePbFromArray(google::protobuf::Message *msg, const void *data, size_t size);

    bool ParsePbFromString(google::protobuf::Message *msg, const std::string &str);

    // Deleter for unique_ptr to print error_text of the controller when
    // -log_error_text is on, then delete the controller if `delete_cntl' is true
    class LogErrorTextAndDelete {
    public:
        explicit LogErrorTextAndDelete(bool delete_cntl = true)
                : _delete_cntl(delete_cntl) {}

        void operator()(Controller *c) const;

    private:
        bool _delete_cntl;
    };

    // Utility to build a temporary array.
    // Example:
    //   TemporaryArrayBuilder<Foo, 5> b;
    //   b.push() = Foo1;
    //   b.push() = Foo2;
    //   UseArray(b.raw_array(), b.size());
    template<typename T, size_t N>
    class TemporaryArrayBuilder {
    public:
        TemporaryArrayBuilder() : _size(0) {}

        T &push() {
            if (_size < N) {
                return _arr[_size++];
            } else {
                CHECK(false) << "push to a full array, cap=" << N;
                static T dummy;
                return dummy;
            }
        }

        T &operator[](size_t i) { return _arr[i]; }

        size_t size() const { return _size; }

        T *raw_array() { return _arr; }

    private:
        size_t _size;
        T _arr[N];
    };

} // namespace melon
