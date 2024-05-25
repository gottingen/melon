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


#include <melon/utility/logging.h>
#include "pb_util.h"

using google::protobuf::ServiceDescriptor;
using google::protobuf::Descriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::MessageFactory;
using google::protobuf::Message;
using google::protobuf::MethodDescriptor;
using google::protobuf::compiler::Importer;
using google::protobuf::DynamicMessageFactory;
using google::protobuf::DynamicMessageFactory;
using std::string;

namespace pbrpcframework {

const MethodDescriptor* find_method_by_name(const string& service_name, 
                                            const string& method_name, 
                                            Importer* importer) {
    const ServiceDescriptor* descriptor =
        importer->pool()->FindServiceByName(service_name);
    if (NULL == descriptor) {
        MLOG(FATAL) << "Fail to find service=" << service_name;
        return NULL;
    }
    return descriptor->FindMethodByName(method_name);
}

const Message* get_prototype_by_method_descriptor(
    const MethodDescriptor* descripter,
    bool is_input, 
    DynamicMessageFactory* factory) {
    if (NULL == descripter) {
        MLOG(FATAL) <<"Param[descripter] is NULL";
        return NULL;
    }   
    const Descriptor* message_descriptor = NULL;
    if (is_input) {
        message_descriptor = descripter->input_type();
    } else {
        message_descriptor = descripter->output_type();
    }   
    return factory->GetPrototype(message_descriptor);
}

const Message* get_prototype_by_name(const string& service_name,
                                     const string& method_name, bool is_input, 
                                     Importer* importer,
                                     DynamicMessageFactory* factory){
    const MethodDescriptor* descripter = find_method_by_name(
        service_name, method_name, importer);
    return get_prototype_by_method_descriptor(descripter, is_input, factory);
}

}  // pbrpcframework
