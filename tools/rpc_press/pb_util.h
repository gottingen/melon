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


#ifndef UTIL_PB_UTIL_H
#define UTIL_PB_UTIL_H
#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/compiler/importer.h>

namespace pbrpcframework {
const google::protobuf::MethodDescriptor* find_method_by_name(
        const std::string& service_name, 
        const std::string& method_name, 
        google::protobuf::compiler::Importer* importer); 

const google::protobuf::Message* get_prototype_by_method_descriptor(
        const google::protobuf::MethodDescriptor* descripter, 
        bool is_input,
        google::protobuf::DynamicMessageFactory* factory);

const google::protobuf::Message* get_prototype_by_name(
        const std::string& service_name, 
        const std::string& method_name, 
        bool is_input, 
        google::protobuf::compiler::Importer* importer, 
        google::protobuf::DynamicMessageFactory* factory);
}
#endif //UTIL_PB_UTIL_H
