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
// Created by jeff on 24-6-27.
//

#include <melon/br/protobufs.h>
#include <collie/nlohmann/json.hpp>
#include <turbo/flags/flag.h>
#include <turbo/flags/reflection.h>
#include <turbo/strings/str_split.h>
#include <turbo/strings/match.h>

namespace melon {

    turbo::Status ListProtobufProcessor::initialize(Server *server) {
        _server = server;
        Server::ServiceMap &services = _server->_fullname_service_map;
        std::vector<const google::protobuf::Descriptor *> stack;
        stack.reserve(services.size() * 3);
        for (Server::ServiceMap::iterator
                     iter = services.begin(); iter != services.end(); ++iter) {
            if (!iter->second.is_user_service()) {
                continue;
            }
            const google::protobuf::ServiceDescriptor *d =
                    iter->second.service->GetDescriptor();
            _map[d->full_name()] = d->DebugString();
            const int method_count = d->method_count();
            for (int j = 0; j < method_count; ++j) {
                const google::protobuf::MethodDescriptor *md = d->method(j);
                stack.push_back(md->input_type());
                stack.push_back(md->output_type());
            }
        }
        while (!stack.empty()) {
            const google::protobuf::Descriptor *d = stack.back();
            stack.pop_back();
            _map[d->full_name()] = d->DebugString();
            for (int i = 0; i < d->field_count(); ++i) {
                const google::protobuf::FieldDescriptor *f = d->field(i);
                if (f->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE ||
                    f->type() == google::protobuf::FieldDescriptor::TYPE_GROUP) {
                    const google::protobuf::Descriptor *sub_d = f->message_type();
                    if (sub_d != d && _map.find(sub_d->full_name()) == _map.end()) {
                        stack.push_back(sub_d);
                    }
                }
            }
        }
        return turbo::OkStatus();
    }

    void ListProtobufProcessor::process(const melon::RestfulRequest *request, melon::RestfulResponse *response) {
        std::string match_str;
        auto *ptr = request->uri().GetQuery("match");
        if (ptr) {
            match_str = *ptr;
        }
        auto flag_list = turbo::get_all_flags();
        response->set_content_json();
        response->set_access_control_all_allow();
        response->set_status_code(200);
        nlohmann::json j;
        j["code"] = 0;
        j["message"] = "success";
        for (const auto &proto : _map) {
            if(!match_str.empty() && !turbo::str_contains(proto.first, match_str)) {
                continue;
            }
            nlohmann::json flag_json;
            flag_json["proto"] = proto.first;
            flag_json["detail"] = proto.second;
            j["protobufs"].push_back(flag_json);
        }
        response->set_body(j.dump());
    }
}  // namespace melon