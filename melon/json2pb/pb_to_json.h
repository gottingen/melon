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
#include <google/protobuf/io/zero_copy_stream.h> // ZeroCopyOutputStream

namespace json2pb {

    enum EnumOption {
        OUTPUT_ENUM_BY_NAME = 0,          // Output enum by its name
        OUTPUT_ENUM_BY_NUMBER = 1,        // Output enum by its value
    };

    struct Pb2JsonOptions {
        Pb2JsonOptions();

        // Control how enum fields are output
        // Default: OUTPUT_ENUM_BY_NAME
        EnumOption enum_option;

        // Use rapidjson::PrettyWriter to generate the json when this option is on.
        // NOTE: currently PrettyWriter is not optimized yet thus the conversion
        // functions may be slower when this option is turned on.
        // Default: false
        bool pretty_json;

        // Convert "repeated { required string key = 1; required string value = 2; }"
        // to a map object of json and vice versa when this option is turned on.
        // Default: true
        bool enable_protobuf_map;

        // Encode the field of type bytes to string in json using base64
        // encoding when this option is turned on.
        // Default: false for melon-internal, true otherwise.
        bool bytes_to_base64;

        // Convert the repeated field that has no entry
        // to a empty array of json when this option is turned on.
        // Default: false
        bool jsonify_empty_array;

        // Whether to always print primitive fields. By default proto3 primitive
        // fields with default values will be omitted in JSON output. For example, an
        // int32 field set to 0 will be omitted. Set this flag to true will override
        // the default behavior and print primitive fields regardless of their values.
        bool always_print_primitive_fields;

        // Convert the single repeated field to a json array when this option is turned on.
        // Default: false.
        bool single_repeated_to_array;
    };

    // Convert protobuf `messge' to `json' according to `options'.
    // Returns true on success. `error' (if not NULL) will be set with error
    // message on failure.
    bool ProtoMessageToJson(const google::protobuf::Message &message,
                            std::string *json,
                            const Pb2JsonOptions &options,
                            std::string *error = NULL);

    // send output to ZeroCopyOutputStream instead of std::string.
    bool ProtoMessageToJson(const google::protobuf::Message &message,
                            google::protobuf::io::ZeroCopyOutputStream *json,
                            const Pb2JsonOptions &options,
                            std::string *error = NULL);

    // Using default Pb2JsonOptions.
    bool ProtoMessageToJson(const google::protobuf::Message &message,
                            std::string *json,
                            std::string *error = NULL);

    bool ProtoMessageToJson(const google::protobuf::Message &message,
                            google::protobuf::io::ZeroCopyOutputStream *json,
                            std::string *error = NULL);
} // namespace json2pb

