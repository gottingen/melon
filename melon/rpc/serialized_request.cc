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



#include <melon/rpc/serialized_request.h>
#include <melon/utility/logging.h>

namespace melon {

    SerializedRequest::SerializedRequest()
            : ::google::protobuf::Message() {
        SharedCtor();
    }

    SerializedRequest::SerializedRequest(const SerializedRequest &from)
            : ::google::protobuf::Message() {
        SharedCtor();
        MergeFrom(from);
    }

    void SerializedRequest::SharedCtor() {
    }

    SerializedRequest::~SerializedRequest() {
        SharedDtor();
    }

    void SerializedRequest::SharedDtor() {
    }

    void SerializedRequest::SetCachedSize(int /*size*/) const {
        MCHECK(false) << "You're not supposed to call " << __FUNCTION__;
    }

    const ::google::protobuf::Descriptor *SerializedRequest::descriptor() {
        return SerializedRequestBase::descriptor();
    }

    SerializedRequest *SerializedRequest::New() const {
        return new SerializedRequest;
    }

#if GOOGLE_PROTOBUF_VERSION >= 3006000

    SerializedRequest *
    SerializedRequest::New(::google::protobuf::Arena *arena) const {
        return CreateMaybeMessage<SerializedRequest>(arena);
    }

#endif

    void SerializedRequest::Clear() {
        _serialized.clear();
    }

    bool SerializedRequest::MergePartialFromCodedStream(
            ::google::protobuf::io::CodedInputStream *) {
        MCHECK(false) << "You're not supposed to call " << __FUNCTION__;
        return false;
    }

    void SerializedRequest::SerializeWithCachedSizes(
            ::google::protobuf::io::CodedOutputStream *) const {
        MCHECK(false) << "You're not supposed to call " << __FUNCTION__;
    }

    ::google::protobuf::uint8 *SerializedRequest::SerializeWithCachedSizesToArray(
            ::google::protobuf::uint8 *target) const {
        MCHECK(false) << "You're not supposed to call " << __FUNCTION__;
        return target;
    }

    int SerializedRequest::ByteSize() const {
        return (int) _serialized.size();
    }

    void SerializedRequest::MergeFrom(const ::google::protobuf::Message &) {
        MCHECK(false) << "You're not supposed to call " << __FUNCTION__;
    }

    void SerializedRequest::MergeFrom(const SerializedRequest &) {
        MCHECK(false) << "You're not supposed to call " << __FUNCTION__;
    }

    void SerializedRequest::CopyFrom(const ::google::protobuf::Message &from) {
        if (&from == this) return;
        const SerializedRequest *source = dynamic_cast<const SerializedRequest *>(&from);
        if (source == NULL) {
            MCHECK(false) << "SerializedRequest can only CopyFrom SerializedRequest";
        } else {
            _serialized = source->_serialized;
        }
    }

    void SerializedRequest::CopyFrom(const SerializedRequest &from) {
        if (&from == this) return;
        _serialized = from._serialized;
    }

    bool SerializedRequest::IsInitialized() const {
        // Always true because it's already serialized.
        return true;
    }

    void SerializedRequest::Swap(SerializedRequest *other) {
        if (other != this) {
            _serialized.swap(other->_serialized);
        }
    }

    ::google::protobuf::Metadata SerializedRequest::GetMetadata() const {
        ::google::protobuf::Metadata metadata;
        metadata.descriptor = SerializedRequest::descriptor();
        metadata.reflection = NULL;
        return metadata;
    }

} // namespace melon
