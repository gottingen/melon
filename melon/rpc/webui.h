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

#include <melon/proto/rpc/webui.pb.h>
#include <melon/rpc/restful_request.h>
#include <melon/rpc/restful_response.h>
#include <turbo/utility/status.h>
#include <melon/rpc/channel.h>
#include <turbo/base/nullability.h>
#include <melon/utility/file_util.h>
#include <turbo/container/flat_hash_map.h>
#include <turbo/container/cache.h>
#include <shared_mutex>
#include <memory>

namespace melon {


    struct WebuiConfig {
        std::string mapping_path;
        std::string root_path;
        std::string index_path;
        std::string not_found_str;
        std::string not_found_path;

        turbo::flat_hash_map<std::string, std::string> headers;

        std::string get_content_type(std::string_view path) const;

        void add_content_type(std::string_view suffix, std::string_view type) {
            content_types[suffix] = type;
        }

        static WebuiConfig default_config();
    private:
        turbo::flat_hash_map<std::string, std::string> content_types;
    };

    class WebuiService : public melon::webui {
    public:
        ~WebuiService() override = default;

        void impl_method(::google::protobuf::RpcController *controller,
                         const ::melon::NoUseWebuiRequest *request,
                         ::melon::NoUseWebuiResponse *response,
                         ::google::protobuf::Closure *done) override;

        turbo::Status register_server(const WebuiConfig &conf, turbo::Nonnull<Server *> server);

        static WebuiService *instance() {
            static WebuiService service;
            return &service;
        }

    private:
        WebuiService();
        void process_not_found(const RestfulRequest *request, RestfulResponse *response);
        mutil::FilePath get_file_meta(const RestfulRequest *request);

        std::shared_ptr<std::string> get_content(const mutil::FilePath &path);
    private:

        WebuiConfig conf_;
        std::mutex mutex_;
        bool registered_ = false;
        std::shared_mutex file_cache_mutex_;
        turbo::LRUCache<std::string, std::string> file_cache_;
    };
}  // namespace melon