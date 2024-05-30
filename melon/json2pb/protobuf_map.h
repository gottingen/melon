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

#include <google/protobuf/descriptor.h>

namespace json2pb {

    const char *const KEY_NAME = "key";
    const char *const VALUE_NAME = "value";
    const int KEY_INDEX = 0;
    const int VALUE_INDEX = 1;

    // Map inside protobuf is officially supported in proto3 using
    // statement like: map<string, string> my_map = N;
    // However, we can still emmulate this map in proto2 by writing:
    // message MapFieldEntry {
    //     required string key = 1;           // MUST be the first
    //     required string value = 2;         // MUST be the second
    // }
    // repeated MapFieldEntry my_map = N;
    //
    // Natually, when converting this map to json, it should be like:
    // { "my_map": {"key1": value1, "key2": value2 } }
    // instead of:
    // { "my_map": [{"key": "key1", "value": value1},
    //              {"key": "key2", "value": value2}] }
    // In order to get the former one, the type of `key' field MUST be
    // string since JSON only supports string keys

    // Check whether `field' is a map type field and is convertable
    bool IsProtobufMap(const google::protobuf::FieldDescriptor *field);

} // namespace json2pb
