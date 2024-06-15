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
#include <deque>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <melon/base/iobuf.h>

namespace melon {

// This utility loads pb messages in json format from a file or string.
class JsonLoader {
public:
    JsonLoader(google::protobuf::compiler::Importer* importer, 
               google::protobuf::DynamicMessageFactory* factory,
               const std::string& service_name,
               const std::string& method_name);
    ~JsonLoader() {}

    // TODO(gejun): messages should be lazily loaded.
    
    // Load jsons from fd or string, convert them into pb messages, then insert
    // them into `out_msgs'.
    void load_messages(int fd, std::deque<google::protobuf::Message*>* out_msgs);
    void load_messages(const std::string& jsons,
                       std::deque<google::protobuf::Message*>* out_msgs);
    
private:
    class Reader;

    void load_messages(
        JsonLoader::Reader* ctx,
        std::deque<google::protobuf::Message*>* out_msgs);

    google::protobuf::compiler::Importer* _importer;
    google::protobuf::DynamicMessageFactory* _factory;
    std::string _service_name;
    std::string _method_name;
    const google::protobuf::Message* _request_prototype;
};

} // namespace melon
