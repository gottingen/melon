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


#ifndef MELON_JSON2PB_PB_TO_JSON_H_
#define MELON_JSON2PB_PB_TO_JSON_H_

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
bool ProtoMessageToJson(const google::protobuf::Message& message,
                        std::string* json,
                        const Pb2JsonOptions& options,
                        std::string* error = NULL);
// send output to ZeroCopyOutputStream instead of std::string.
bool ProtoMessageToJson(const google::protobuf::Message& message,
                        google::protobuf::io::ZeroCopyOutputStream *json,
                        const Pb2JsonOptions& options,
                        std::string* error = NULL);

// Using default Pb2JsonOptions.
bool ProtoMessageToJson(const google::protobuf::Message& message,
                        std::string* json,
                        std::string* error = NULL);
bool ProtoMessageToJson(const google::protobuf::Message& message,
                        google::protobuf::io::ZeroCopyOutputStream* json,
                        std::string* error = NULL);
} // namespace json2pb

#endif // MELON_JSON2PB_PB_TO_JSON_H_
