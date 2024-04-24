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



#ifndef MELON_RPC_SERIALIZED_REQUEST_H_
#define MELON_RPC_SERIALIZED_REQUEST_H_

#include <google/protobuf/message.h>
#include "melon/utility/iobuf.h"
#include "melon/proto/rpc/proto_base.pb.h"
#include "melon/rpc/pb_compat.h"

namespace melon {

class SerializedRequest : public ::google::protobuf::Message {
public:
    SerializedRequest();
    virtual ~SerializedRequest();
  
    SerializedRequest(const SerializedRequest& from);
  
    inline SerializedRequest& operator=(const SerializedRequest& from) {
        CopyFrom(from);
        return *this;
    }
  
    static const ::google::protobuf::Descriptor* descriptor();
  
    void Swap(SerializedRequest* other);
  
    // implements Message ----------------------------------------------
  
    SerializedRequest* New() const PB_319_OVERRIDE;
#if GOOGLE_PROTOBUF_VERSION >= 3006000
    SerializedRequest* New(::google::protobuf::Arena* arena) const override;
#endif
    void CopyFrom(const ::google::protobuf::Message& from) PB_321_OVERRIDE;
    void CopyFrom(const SerializedRequest& from);
    void Clear() override;
    bool IsInitialized() const override;
    int ByteSize() const;
    int GetCachedSize() const override { return (int)_serialized.size(); }
    mutil::IOBuf& serialized_data() { return _serialized; }
    const mutil::IOBuf& serialized_data() const { return _serialized; }

protected:
    ::google::protobuf::Metadata GetMetadata() const override;
    
private:
    bool MergePartialFromCodedStream(
        ::google::protobuf::io::CodedInputStream* input) PB_310_OVERRIDE;
    void SerializeWithCachedSizes(
        ::google::protobuf::io::CodedOutputStream* output) const PB_310_OVERRIDE;
    ::google::protobuf::uint8* SerializeWithCachedSizesToArray(
        ::google::protobuf::uint8* output) const PB_310_OVERRIDE;
    void MergeFrom(const ::google::protobuf::Message& from) override;
    void MergeFrom(const SerializedRequest& from);
    void SharedCtor();
    void SharedDtor();
    void SetCachedSize(int size) const override;
  
private:
    mutil::IOBuf _serialized;
};

} // namespace melon


#endif  // MELON_RPC_SERIALIZED_REQUEST_H_
