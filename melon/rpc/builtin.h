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
// Created by jeff on 24-6-14.
//

#pragma once

#include <melon/proto/rpc/builtin.pb.h>
#include <melon/rpc/restful_request.h>
#include <melon/rpc/restful_response.h>
#include <turbo/utility/status.h>
#include <turbo/container/flat_hash_map.h>
#include <melon/rpc/channel.h>
#include <memory>
#include <mutex>
#include <vector>

namespace melon {

    struct BuiltinProcessor {
        virtual ~BuiltinProcessor() = default;
        virtual void process(const RestfulRequest *request, RestfulResponse *response) = 0;

        void add_eternal_header(const std::string &key, const std::string &value) {
            eternal_headers.emplace_back(key, value);
        }

        void apply_eternal_headers(RestfulResponse *response) {
            for(const auto &header : eternal_headers) {
                response->set_header(header.first, header.second);
            }
        }
        void clear_eternal_headers() {
            eternal_headers.clear();
        }
        std::vector<std::pair<std::string, std::string>> eternal_headers;
    };

    class BuiltinRestful : public melon::builtin {
    public:
        ~BuiltinRestful() override = default;

        void impl_method(::google::protobuf::RpcController *controller,
                         const ::melon::NoUseBuiltinRequest *request,
                         ::melon::NoUseBuiltinResponse *response,
                         ::google::protobuf::Closure *done) override;

        turbo::Status register_server(Server *server);

        BuiltinRestful* set_not_found_processor(std::shared_ptr<BuiltinProcessor> processor);

        BuiltinRestful* set_any_path_processor(std::shared_ptr<BuiltinProcessor> processor);

        BuiltinRestful* set_root_processor(std::shared_ptr<BuiltinProcessor> processor);

        BuiltinRestful* set_processor(const std::string &path, std::shared_ptr<BuiltinProcessor> processor, bool overwrite = false);

        BuiltinRestful* set_mapping_path(const std::string &mapping_path);

        static BuiltinRestful *instance() {
            static BuiltinRestful service;
            return &service;
        }
    private:
        BuiltinRestful() = default;
    private:
        std::mutex mutex_;
        bool registered_{false};
        std::string mapping_path_;
        // for root path
        std::shared_ptr<BuiltinProcessor> root_processor_;
        // for any path
        std::shared_ptr<BuiltinProcessor> any_path_processor_;
        // for not found
        std::shared_ptr<BuiltinProcessor> not_found_processor_;
        turbo::flat_hash_map<std::string, std::shared_ptr<BuiltinProcessor>> processors_;
    };
}  // namespace melon