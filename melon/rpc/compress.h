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

#include <google/protobuf/message.h>              // Message
#include <melon/utility/iobuf.h>                           // mutil::IOBuf
#include <melon/proto/rpc/options.pb.h>                     // CompressType

namespace melon {

    struct CompressHandler {
        // Compress serialized `msg' into `buf'.
        // Returns true on success, false otherwise
        bool (*Compress)(const google::protobuf::Message &msg, mutil::IOBuf *buf);

        // Parse decompressed `data' as `msg'.
        // Returns true on success, false otherwise
        bool (*Decompress)(const mutil::IOBuf &data, google::protobuf::Message *msg);

        // Name of the compression algorithm, must be string constant.
        const char *name;
    };

    // [NOT thread-safe] Register `handler' using key=`type'
    // Returns 0 on success, -1 otherwise
    int RegisterCompressHandler(CompressType type, CompressHandler handler);

    // Returns the `name' of the CompressType if registered
    const char *CompressTypeToCStr(CompressType type);

    // Put all registered handlers into `vec'.
    void ListCompressHandler(std::vector<CompressHandler> *vec);

    // Parse decompressed `data' as `msg' using registered `compress_type'.
    // Returns true on success, false otherwise
    bool ParseFromCompressedData(const mutil::IOBuf &data,
                                 google::protobuf::Message *msg,
                                 CompressType compress_type);

    // Compress serialized `msg' into `buf' using registered `compress_type'.
    // Returns true on success, false otherwise
    bool SerializeAsCompressedData(const google::protobuf::Message &msg,
                                   mutil::IOBuf *buf,
                                   CompressType compress_type);

} // namespace melon
