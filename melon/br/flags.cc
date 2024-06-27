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

#include <melon/br/flags.h>
#include <collie/nlohmann/json.hpp>
#include <turbo/flags/flag.h>
#include <turbo/flags/reflection.h>
#include <turbo/strings/str_split.h>
#include <turbo/strings/match.h>

TURBO_FLAG(bool, tflags_immable, false, "test immable flag");

namespace melon {

    static std::string trim_path(const std::string &path) {
        std::vector<std::string> parts = turbo::str_split(path, "/", turbo::SkipEmpty());
        // remove too many prefix path, max depth is 3
        size_t index = 0;
        size_t count = 0;
        if (parts.size() > 3) {
            index = parts.size() - 3;
            count = 3;
        } else {
            count = parts.size();
        }
        if(count >= 2) {
            if(parts[index] == parts[index + 1]) {
                index++;
                count--;
            }
        }
        std::string result;
        bool first = true;
        for (size_t i = index; i < index + count; i++) {
            if (!first) {
                result += "/";
            }
            result += parts[i];
            first = false;
        }
        return result;
    }

    void ListFlagsProcessor::process(const melon::RestfulRequest *request, melon::RestfulResponse *response) {
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
        for (const auto &flag : flag_list) {
            if(!match_str.empty() && !turbo::str_contains(flag.second->name(), match_str)) {
                continue;
            }
            nlohmann::json flag_json;
            flag_json["name"] = flag.second->name();
            flag_json["reset_able"] = flag.second->has_user_validator();
            flag_json["default_value"] = flag.second->default_value();
            flag_json["current_value"] = flag.second->current_value();
            flag_json["help"] = flag.second->help();
            flag_json["file"] = trim_path(flag.second->filename());
            j["flags"].push_back(flag_json);
        }
        response->set_body(j.dump());
    }

    void ResetFlagsProcessor::process(const melon::RestfulRequest *request, melon::RestfulResponse *response) {
        auto flag_list = turbo::get_all_flags();
        std::string input_str = request->body().to_string();
        response->set_content_json();
        response->set_access_control_all_allow();
        response->set_status_code(200);
        nlohmann::json input;
        try {
            input = nlohmann::json::parse(input_str);
        } catch (const std::exception &e) {
            nlohmann::json j;
            j["code"] = static_cast<int>(turbo::StatusCode::kInvalidArgument);
            j["message"] = "invalid json";
            response->set_body(j.dump());
            return;
        }
        std::string name;
        if (input.find("name") != input.end()) {
            name = input["name"].get<std::string>();
        } else {
            nlohmann::json j;
            j["code"] = static_cast<int>(turbo::StatusCode::kInvalidArgument);
            j["message"] = "name is required";
            response->set_body(j.dump());
            return;
        }

        std::string value;
        if (input.find("value") != input.end()) {
            value = input["value"].get<std::string>();
        } else {
            nlohmann::json j;
            j["code"] = static_cast<int>(turbo::StatusCode::kInvalidArgument);
            j["message"] = "value is required";
            response->set_body(j.dump());
            return;
        }
        auto fflag = turbo::find_command_line_flag(name);
        if (fflag == nullptr) {
            nlohmann::json j;
            j["code"] = static_cast<int>(turbo::StatusCode::kNotFound);
            j["message"] = "flag not found";
            response->set_body(j.dump());
            return;
        }
        if(!fflag->has_user_validator()){
            nlohmann::json j;
            j["code"] = static_cast<int>(turbo::StatusCode::kInvalidArgument);
            j["message"] = "flag is not resetable";
            response->set_body(j.dump());
            return;
        }

        if(turbo::get_flag(FLAGS_tflags_immable)) {
            nlohmann::json j;
            j["code"] = static_cast<int>(turbo::StatusCode::kInvalidArgument);
            j["message"] = "global config flags is immable";
            response->set_body(j.dump());
            return;
        }
        std::string error;
        auto r = fflag->user_validate(value, &error);
        if (!r) {
            nlohmann::json j;
            j["code"] = static_cast<int>(turbo::StatusCode::kInvalidArgument);
            j["message"] = error;
            response->set_body(j.dump());
            return;
        }

        fflag->parse_from(value, &error);
        nlohmann::json j;
        j["code"] = 0;
        j["message"] = "ok";
        response->set_body(j.dump());
    }
}  // namespace melon