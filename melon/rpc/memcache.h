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


#ifndef MELON_RPC_MEMCACHE_H_
#define MELON_RPC_MEMCACHE_H_

#include <string>
#include <google/protobuf/message.h>

#include "melon/io/cord_buf.h"
#include <string_view>
#include "melon/rpc/proto_base.pb.h"

namespace melon::rpc {

    // Request to memcache.
    // Notice that you can pipeline multiple operations in one request and sent
    // them to memcached server together.
    // Example:
    //   MemcacheRequest request;
    //   request.get("my_key1");
    //   request.get("my_key2");
    //   request.set("my_key3", "some_value", 0, 10);
    //   ...
    //   MemcacheResponse response;
    //   // 2 GET and 1 SET are sent to the server together.
    //   channel.CallMethod(&controller, &request, &response, nullptr/*done*/);
    class MemcacheRequest : public ::google::protobuf::Message {
    public:
        MemcacheRequest();

        virtual ~MemcacheRequest();

        MemcacheRequest(const MemcacheRequest &from);

        inline MemcacheRequest &operator=(const MemcacheRequest &from) {
            CopyFrom(from);
            return *this;
        }

        void Swap(MemcacheRequest *other);

        bool Get(const std::string_view &key);

        // If the cas_value(Data Version Check) is non-zero, the requested operation
        // MUST only succeed if the item exists and has a cas_value identical to the
        // provided value.
        bool Set(const std::string_view &key, const std::string_view &value,
                 uint32_t flags, uint32_t exptime, uint64_t cas_value);

        bool Add(const std::string_view &key, const std::string_view &value,
                 uint32_t flags, uint32_t exptime, uint64_t cas_value);

        bool Replace(const std::string_view &key, const std::string_view &value,
                     uint32_t flags, uint32_t exptime, uint64_t cas_value);

        bool Append(const std::string_view &key, const std::string_view &value,
                    uint32_t flags, uint32_t exptime, uint64_t cas_value);

        bool Prepend(const std::string_view &key, const std::string_view &value,
                     uint32_t flags, uint32_t exptime, uint64_t cas_value);

        bool Delete(const std::string_view &key);

        bool Flush(uint32_t timeout);

        bool Increment(const std::string_view &key, uint64_t delta,
                       uint64_t initial_value, uint32_t exptime);

        bool Decrement(const std::string_view &key, uint64_t delta,
                       uint64_t initial_value, uint32_t exptime);

        bool Touch(const std::string_view &key, uint32_t exptime);

        bool Version();

        int pipelined_count() const { return _pipelined_count; }

        melon::cord_buf &raw_buffer() { return _buf; }

        const melon::cord_buf &raw_buffer() const { return _buf; }

        // Protobuf methods.
        MemcacheRequest *New() const;

        MemcacheRequest *New(::google::protobuf::Arena *arena) const override;

        void CopyFrom(const ::google::protobuf::Message &from) override;

        void MergeFrom(const ::google::protobuf::Message &from) override;

        void CopyFrom(const MemcacheRequest &from);

        void MergeFrom(const MemcacheRequest &from);

        void Clear() override;

        bool IsInitialized() const override;

        int ByteSize() const;

        bool MergePartialFromCodedStream(
                ::google::protobuf::io::CodedInputStream *input);

        void SerializeWithCachedSizes(
                ::google::protobuf::io::CodedOutputStream *output) const;

        ::google::protobuf::uint8 *SerializeWithCachedSizesToArray(::google::protobuf::uint8 *output) const;

        int GetCachedSize() const override { return _cached_size_; }

        static const ::google::protobuf::Descriptor *descriptor();

    protected:

        ::google::protobuf::Metadata GetMetadata() const override;

    private:

        bool GetOrDelete(uint8_t command, const std::string_view &key);

        bool Counter(uint8_t command, const std::string_view &key, uint64_t delta,
                     uint64_t initial_value, uint32_t exptime);

        bool Store(uint8_t command, const std::string_view &key,
                   const std::string_view &value,
                   uint32_t flags, uint32_t exptime, uint64_t cas_value);

        void SharedCtor();

        void SharedDtor();

        void SetCachedSize(int size) const override;

        int _pipelined_count;
        melon::cord_buf _buf;
        mutable int _cached_size_;
    };

    // Response from Memcache.
    // Notice that a MemcacheResponse instance may contain multiple operations
    // due to pipelining. You can call pop_xxx according to your calling sequence
    // of operations in corresponding MemcacheRequest.
    // Example:
    //   MemcacheResponse response;
    //   channel.CallMethod(&controller, &request, &response, nullptr/*done*/);
    //   ...
    //   if (!response.PopGet(&my_value1, &flags1, &cas1)) {
    //       MELON_LOG(FATAL) << "Fail to pop GET: " << response.LastError();
    //   } else {
    //       // Use my_value1, flags1, cas1
    //   }
    //   if (!response.PopGet(&my_value2, &flags2, &cas2)) {
    //       MELON_LOG(FATAL) << "Fail to pop GET: " << response.LastError();
    //   } else {
    //       // Use my_value2, flags2, cas2
    //   }
    //   if (!response.PopSet(&cas3)) {
    //       MELON_LOG(FATAL) << "Fail to pop SET: " << response.LastError();
    //   } else {
    //       // the SET was successful.
    //   }
    class MemcacheResponse : public ::google::protobuf::Message {
    public:
        // Definition of the valid response status numbers.
        // See section 3.2 Response Status
        enum Status {
            STATUS_SUCCESS = 0x00,
            STATUS_KEY_ENOENT = 0x01,
            STATUS_KEY_EEXISTS = 0x02,
            STATUS_E2BIG = 0x03,
            STATUS_EINVAL = 0x04,
            STATUS_NOT_STORED = 0x05,
            STATUS_DELTA_BADVAL = 0x06,
            STATUS_AUTH_ERROR = 0x20,
            STATUS_AUTH_CONTINUE = 0x21,
            STATUS_UNKNOWN_COMMAND = 0x81,
            STATUS_ENOMEM = 0x82
        };

        MemcacheResponse();

        virtual ~MemcacheResponse();

        MemcacheResponse(const MemcacheResponse &from);

        inline MemcacheResponse &operator=(const MemcacheResponse &from) {
            CopyFrom(from);
            return *this;
        }

        void Swap(MemcacheResponse *other);

        const std::string &LastError() const { return _err; }

        bool PopGet(melon::cord_buf *value, uint32_t *flags, uint64_t *cas_value);

        bool PopGet(std::string *value, uint32_t *flags, uint64_t *cas_value);

        bool PopSet(uint64_t *cas_value);

        bool PopAdd(uint64_t *cas_value);

        bool PopReplace(uint64_t *cas_value);

        bool PopAppend(uint64_t *cas_value);

        bool PopPrepend(uint64_t *cas_value);

        bool PopDelete();

        bool PopFlush();

        bool PopIncrement(uint64_t *new_value, uint64_t *cas_value);

        bool PopDecrement(uint64_t *new_value, uint64_t *cas_value);

        bool PopTouch();

        bool PopVersion(std::string *version);

        melon::cord_buf &raw_buffer() { return _buf; }

        const melon::cord_buf &raw_buffer() const { return _buf; }

        static const char *status_str(Status);

        // implements Message ----------------------------------------------

        MemcacheResponse *New() const;

        MemcacheResponse *New(::google::protobuf::Arena *arena) const override;

        void CopyFrom(const ::google::protobuf::Message &from) override;

        void MergeFrom(const ::google::protobuf::Message &from) override;

        void CopyFrom(const MemcacheResponse &from);

        void MergeFrom(const MemcacheResponse &from);

        void Clear() override;

        bool IsInitialized() const override;

        int ByteSize() const;

        bool MergePartialFromCodedStream(
                ::google::protobuf::io::CodedInputStream *input);

        void SerializeWithCachedSizes(
                ::google::protobuf::io::CodedOutputStream *output) const;

        ::google::protobuf::uint8 *SerializeWithCachedSizesToArray(::google::protobuf::uint8 *output) const;

        int GetCachedSize() const override { return _cached_size_; }

        static const ::google::protobuf::Descriptor *descriptor();

    protected:

        ::google::protobuf::Metadata GetMetadata() const override;

    private:

        bool PopCounter(uint8_t command, uint64_t *new_value, uint64_t *cas_value);

        bool PopStore(uint8_t command, uint64_t *cas_value);

        void SharedCtor();

        void SharedDtor();

        void SetCachedSize(int size) const override;

        std::string _err;
        melon::cord_buf _buf;
        mutable int _cached_size_;
    };

} // namespace melon::rpc


#endif  // MELON_RPC_MEMCACHE_H_
