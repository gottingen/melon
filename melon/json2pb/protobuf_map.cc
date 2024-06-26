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

#include <melon/json2pb/protobuf_map.h>
#include <stdio.h>

namespace json2pb {

    using google::protobuf::Descriptor;
    using google::protobuf::FieldDescriptor;

    bool IsProtobufMap(const FieldDescriptor *field) {
        if (field->type() != FieldDescriptor::TYPE_MESSAGE || !field->is_repeated()) {
            return false;
        }
        const Descriptor *entry_desc = field->message_type();
        if (entry_desc == NULL) {
            return false;
        }
        if (entry_desc->field_count() != 2) {
            return false;
        }
        const FieldDescriptor *key_desc = entry_desc->field(KEY_INDEX);
        if (NULL == key_desc
            || key_desc->is_repeated()
            || key_desc->cpp_type() != FieldDescriptor::CPPTYPE_STRING
            || strcmp(KEY_NAME, key_desc->name().c_str()) != 0) {
            return false;
        }
        const FieldDescriptor *value_desc = entry_desc->field(VALUE_INDEX);
        if (NULL == value_desc
            || strcmp(VALUE_NAME, value_desc->name().c_str()) != 0) {
            return false;
        }
        return true;
    }

} // namespace json2pb
