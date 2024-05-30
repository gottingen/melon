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

#include <string>
#include <google/protobuf/message.h>

#include <melon/utility/iobuf.h>
#include <melon/utility/strings/string_piece.h>
#include <melon/proto/rpc/proto_base.pb.h>
#include <melon/rpc/pb_compat.h>

namespace melon {

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
//   channel.CallMethod(&controller, &request, &response, NULL/*done*/);
class MemcacheRequest : public ::google::protobuf::Message {
public:
    MemcacheRequest();
    virtual ~MemcacheRequest();
    MemcacheRequest(const MemcacheRequest& from);
    inline MemcacheRequest& operator=(const MemcacheRequest& from) {
        CopyFrom(from);
        return *this;
    }
    void Swap(MemcacheRequest* other);

    bool Get(const mutil::StringPiece& key);

    // If the cas_value(Data Version Check) is non-zero, the requested operation
    // MUST only succeed if the item exists and has a cas_value identical to the
    // provided value.
    bool Set(const mutil::StringPiece& key, const mutil::StringPiece& value,
             uint32_t flags, uint32_t exptime, uint64_t cas_value);
    
    bool Add(const mutil::StringPiece& key, const mutil::StringPiece& value,
             uint32_t flags, uint32_t exptime, uint64_t cas_value);

    bool Replace(const mutil::StringPiece& key, const mutil::StringPiece& value,
                 uint32_t flags, uint32_t exptime, uint64_t cas_value);
    
    bool Append(const mutil::StringPiece& key, const mutil::StringPiece& value,
                uint32_t flags, uint32_t exptime, uint64_t cas_value);

    bool Prepend(const mutil::StringPiece& key, const mutil::StringPiece& value,
                 uint32_t flags, uint32_t exptime, uint64_t cas_value);

    bool Delete(const mutil::StringPiece& key);
    bool Flush(uint32_t timeout);

    bool Increment(const mutil::StringPiece& key, uint64_t delta,
                   uint64_t initial_value, uint32_t exptime);
    bool Decrement(const mutil::StringPiece& key, uint64_t delta,
                   uint64_t initial_value, uint32_t exptime);
    
    bool Touch(const mutil::StringPiece& key, uint32_t exptime);

    bool Version();

    int pipelined_count() const { return _pipelined_count; }

    mutil::IOBuf& raw_buffer() { return _buf; }
    const mutil::IOBuf& raw_buffer() const { return _buf; }

    // Protobuf methods.
    MemcacheRequest* New() const PB_319_OVERRIDE;
#if GOOGLE_PROTOBUF_VERSION >= 3006000
    MemcacheRequest* New(::google::protobuf::Arena* arena) const override;
#endif
    void CopyFrom(const ::google::protobuf::Message& from) PB_321_OVERRIDE;
    void MergeFrom(const ::google::protobuf::Message& from) override;
    void CopyFrom(const MemcacheRequest& from);
    void MergeFrom(const MemcacheRequest& from);
    void Clear() override;
    bool IsInitialized() const override;
  
    int ByteSize() const;
    bool MergePartialFromCodedStream(
        ::google::protobuf::io::CodedInputStream* input) PB_310_OVERRIDE;
    void SerializeWithCachedSizes(
        ::google::protobuf::io::CodedOutputStream* output) const PB_310_OVERRIDE;
    ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const PB_310_OVERRIDE;
    int GetCachedSize() const override { return _cached_size_; }
    
    static const ::google::protobuf::Descriptor* descriptor();

protected:
    ::google::protobuf::Metadata GetMetadata() const override;
    
private:
    bool GetOrDelete(uint8_t command, const mutil::StringPiece& key);
    bool Counter(uint8_t command, const mutil::StringPiece& key, uint64_t delta,
                 uint64_t initial_value, uint32_t exptime);
    
    bool Store(uint8_t command, const mutil::StringPiece& key,
               const mutil::StringPiece& value,
               uint32_t flags, uint32_t exptime, uint64_t cas_value);

    void SharedCtor();
    void SharedDtor();
    void SetCachedSize(int size) const override;

    int _pipelined_count;
    mutil::IOBuf _buf;
    mutable int _cached_size_;
};

// Response from Memcache.
// Notice that a MemcacheResponse instance may contain multiple operations
// due to pipelining. You can call pop_xxx according to your calling sequence
// of operations in corresponding MemcacheRequest.
// Example:
//   MemcacheResponse response;
//   channel.CallMethod(&controller, &request, &response, NULL/*done*/);
//   ...
//   if (!response.PopGet(&my_value1, &flags1, &cas1)) {
//       LOG(FATAL) << "Fail to pop GET: " << response.LastError();
//   } else {
//       // Use my_value1, flags1, cas1
//   }
//   if (!response.PopGet(&my_value2, &flags2, &cas2)) {
//       LOG(FATAL) << "Fail to pop GET: " << response.LastError();
//   } else {
//       // Use my_value2, flags2, cas2
//   }
//   if (!response.PopSet(&cas3)) {
//       LOG(FATAL) << "Fail to pop SET: " << response.LastError();
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
    MemcacheResponse(const MemcacheResponse& from);
    inline MemcacheResponse& operator=(const MemcacheResponse& from) {
        CopyFrom(from);
        return *this;
    }
    void Swap(MemcacheResponse* other);

    const std::string& LastError() const { return _err; }
   
    bool PopGet(mutil::IOBuf* value, uint32_t* flags, uint64_t* cas_value);
    bool PopGet(std::string* value, uint32_t* flags, uint64_t* cas_value);
    bool PopSet(uint64_t* cas_value);
    bool PopAdd(uint64_t* cas_value);
    bool PopReplace(uint64_t* cas_value);
    bool PopAppend(uint64_t* cas_value);
    bool PopPrepend(uint64_t* cas_value);
    bool PopDelete();
    bool PopFlush();
    bool PopIncrement(uint64_t* new_value, uint64_t* cas_value);
    bool PopDecrement(uint64_t* new_value, uint64_t* cas_value);
    bool PopTouch();
    bool PopVersion(std::string* version);
    mutil::IOBuf& raw_buffer() { return _buf; }
    const mutil::IOBuf& raw_buffer() const { return _buf; }
    static const char* status_str(Status);
      
    // implements Message ----------------------------------------------
  
    MemcacheResponse* New() const PB_319_OVERRIDE;
#if GOOGLE_PROTOBUF_VERSION >= 3006000
    MemcacheResponse* New(::google::protobuf::Arena* arena) const override;
#endif
    void CopyFrom(const ::google::protobuf::Message& from) PB_321_OVERRIDE;
    void MergeFrom(const ::google::protobuf::Message& from) override;
    void CopyFrom(const MemcacheResponse& from);
    void MergeFrom(const MemcacheResponse& from);
    void Clear() override;
    bool IsInitialized() const override;
  
    int ByteSize() const;
    bool MergePartialFromCodedStream(
        ::google::protobuf::io::CodedInputStream* input) PB_310_OVERRIDE;
    void SerializeWithCachedSizes(
        ::google::protobuf::io::CodedOutputStream* output) const PB_310_OVERRIDE;
    ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const PB_310_OVERRIDE;
    int GetCachedSize() const override { return _cached_size_; }

    static const ::google::protobuf::Descriptor* descriptor();

protected:
    ::google::protobuf::Metadata GetMetadata() const override;

private:
    bool PopCounter(uint8_t command, uint64_t* new_value, uint64_t* cas_value);
    bool PopStore(uint8_t command, uint64_t* cas_value);

    void SharedCtor();
    void SharedDtor();
    void SetCachedSize(int size) const override;

    std::string _err;
    mutil::IOBuf _buf;
    mutable int _cached_size_;
};

} // namespace melon
