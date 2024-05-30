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

#include <melon/json2pb/zero_copy_stream_reader.h>
#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream.h>    // ZeroCopyInputStream

namespace json2pb {

    struct Json2PbOptions {
        Json2PbOptions();

        // Decode string in json using base64 decoding if the type of
        // corresponding field is bytes when this option is turned on.
        // Default: false for melon-interal, true otherwise.
        bool base64_to_bytes;

        // Allow decoding json array iff there is only one repeated field.
        // Default: false.
        bool array_to_single_repeated;

        // Allow more bytes remaining in the input after parsing the first json
        // object. Useful when the input contains more than one json object.
        bool allow_remaining_bytes_after_parsing;
    };

    // Convert `json' to protobuf `message'.
    // Returns true on success. `error' (if not NULL) will be set with error
    // message on failure.
    //
    // [When options.allow_remaining_bytes_after_parsing is true]
    // * `parse_offset' will be set with #bytes parsed
    // * the function still returns false on empty document but the `error' is set
    //   to empty string instead of `The document is empty'.
    bool JsonToProtoMessage(const std::string &json,
                            google::protobuf::Message *message,
                            const Json2PbOptions &options,
                            std::string *error = nullptr,
                            size_t *parsed_offset = nullptr);

    // Use ZeroCopyInputStream as input instead of std::string.
    bool JsonToProtoMessage(google::protobuf::io::ZeroCopyInputStream *json,
                            google::protobuf::Message *message,
                            const Json2PbOptions &options,
                            std::string *error = nullptr,
                            size_t *parsed_offset = nullptr);

    // Use ZeroCopyStreamReader as input instead of std::string.
    // If you need to parse multiple jsons from IOBuf, you should use this
    // overload instead of the ZeroCopyInputStream one which bases on this
    // and recreates a ZeroCopyStreamReader internally that can't be reused
    // between continuous calls.
    bool JsonToProtoMessage(ZeroCopyStreamReader *json,
                            google::protobuf::Message *message,
                            const Json2PbOptions &options,
                            std::string *error = nullptr,
                            size_t *parsed_offset = nullptr);

    // Using default Json2PbOptions.
    bool JsonToProtoMessage(const std::string &json,
                            google::protobuf::Message *message,
                            std::string *error = nullptr);

    bool JsonToProtoMessage(google::protobuf::io::ZeroCopyInputStream *stream,
                            google::protobuf::Message *message,
                            std::string *error = nullptr);
} // namespace json2pb

