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

#include <google/protobuf/message.h>
#include <melon/utility/iobuf.h>
#include <melon/proto/rpc/proto_base.pb.h>
#include <melon/rpc/pb_compat.h>

namespace melon {

    class SerializedRequest : public ::google::protobuf::Message {
    public:
        SerializedRequest();

        virtual ~SerializedRequest();

        SerializedRequest(const SerializedRequest &from);

        inline SerializedRequest &operator=(const SerializedRequest &from) {
            CopyFrom(from);
            return *this;
        }

        static const ::google::protobuf::Descriptor *descriptor();

        void Swap(SerializedRequest *other);

        // implements Message ----------------------------------------------

        SerializedRequest *New() const PB_319_OVERRIDE;

#if GOOGLE_PROTOBUF_VERSION >= 3006000

        SerializedRequest *New(::google::protobuf::Arena *arena) const override;

#endif

        void CopyFrom(const ::google::protobuf::Message &from) PB_321_OVERRIDE;

        void CopyFrom(const SerializedRequest &from);

        void Clear() override;

        bool IsInitialized() const override;

        int ByteSize() const;

        int GetCachedSize() const override { return (int) _serialized.size(); }

        mutil::IOBuf &serialized_data() { return _serialized; }

        const mutil::IOBuf &serialized_data() const { return _serialized; }

    protected:
        ::google::protobuf::Metadata GetMetadata() const override;

    private:
        bool MergePartialFromCodedStream(
                ::google::protobuf::io::CodedInputStream *input) PB_310_OVERRIDE;

        void SerializeWithCachedSizes(
                ::google::protobuf::io::CodedOutputStream *output) const PB_310_OVERRIDE;

        ::google::protobuf::uint8 *SerializeWithCachedSizesToArray(
                ::google::protobuf::uint8 *output) const PB_310_OVERRIDE;

        void MergeFrom(const ::google::protobuf::Message &from) override;

        void MergeFrom(const SerializedRequest &from);

        void SharedCtor();

        void SharedDtor();

        void SetCachedSize(int size) const override;

    private:
        mutil::IOBuf _serialized;
    };

} // namespace melon

