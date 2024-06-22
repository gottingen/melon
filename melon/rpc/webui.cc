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
#include <melon/rpc/webui.h>
#include <turbo/strings/str_format.h>
#include <melon/rpc/server.h>
#include <turbo/log/logging.h>
#include <turbo/strings/str_split.h>

namespace melon {

    const std::string not_found =
R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>404 Not Found</title>
</head>
<body>
<h1>404 Not Found</h1>
</body>
</html>)";

    WebuiConfig WebuiConfig::default_config() {
        WebuiConfig conf;
        conf.mapping_path = "/webui";
        conf.root_path = "/var/www";
        conf.index_path = "index.html";
        conf.not_found_str = not_found;
        conf.not_found_path = "";
        conf.content_types["html"] = "text/html; charset=utf-8";
        conf.content_types["css"] = "text/css; charset=utf-8";
        conf.content_types["js"] = "application/javascript; charset=utf-8";
        conf.content_types["png"] = "image/png";
        conf.content_types["jpg"] = "image/jpeg";
        conf.content_types["jpeg"] = "image/jpeg";
        conf.content_types["gif"] = "image/gif";
        conf.content_types["ico"] = "image/x-icon";
        conf.content_types["svg"] = "image/svg+xml";
        conf.content_types["ttf"] = "font/ttf";
        conf.content_types["woff"] = "font/woff";
        conf.content_types["woff2"] = "font/woff2";
        conf.content_types["eot"] = "font/eot";
        conf.content_types["otf"] = "font/otf";
        conf.content_types["json"] = "application/json";
        conf.content_types["xml"] = "application/xml";
        conf.content_types["pdf"] = "application/pdf";
        conf.content_types["zip"] = "application/zip";
        conf.content_types["tar"] = "application/x-tar";
        conf.content_types["gz"] = "application/x-gzip";
        conf.content_types["bz2"] = "application/x-bzip2";
        conf.content_types["7z"] = "application/x-7z-compressed";
        conf.content_types["rar"] = "application/x-rar-compressed";
        conf.headers["Server"] = "melon";
        conf.headers["Access-Control-Allow-Origin"] = "*";

        return conf;
    }

    static std::string file_extension(const std::string &path) {
        std::string result;
        std::vector<std::string_view> segments = turbo::str_split(path, ".", turbo::SkipEmpty());
        if(segments.size() <= 1) {
            return "";
        }
        bool first = true;
        for (size_t i = 1; i < segments.size(); i++) {
            if(first) {
                first = false;
            } else {
                result.push_back('.');
            }
            result.append(segments[i]);
        }
        return result;
    }

    WebuiService::WebuiService(): file_cache_(1024) {

    }
    void WebuiService::impl_method(::google::protobuf::RpcController *controller,
                                   const ::melon::NoUseWebuiRequest *request,
                                   ::melon::NoUseWebuiResponse *response,
                                   ::google::protobuf::Closure *done) {
        (void) request;
        (void) response;
        melon::ClosureGuard done_guard(done);
        auto *ctrl = dynamic_cast<Controller *>(controller);
        const RestfulRequest req(ctrl);
        RestfulResponse resp(ctrl);
        auto file_path = get_file_meta(&req);
        std::string extension = file_extension(file_path.value());

        if(!mutil::PathExists(file_path)) {
            process_not_found(&req, &resp);
        } else {
            auto content = get_content(file_path);
            if(content) {
                resp.set_status_code(200);
                resp.set_body(*content);
                resp.set_content_type(conf_.get_content_type(extension));
            } else {
                process_not_found(&req, &resp);
            }
        }
        // process headers
        for(auto &it : conf_.headers) {
            resp.set_header(it.first, it.second);
        }
    }

    turbo::Status WebuiService::register_server(const WebuiConfig &conf, turbo::Nonnull<Server *> server) {
        std::unique_lock lock(mutex_);
        if (registered_) {
            return turbo::already_exists_error("already registered");
        }
        conf_ = conf;
        if (conf_.mapping_path.empty()) {
            return turbo::invalid_argument_error("mapping_path is empty");
        }
        std::string map_str = turbo::str_format("%s/* => impl_method", conf_.mapping_path.c_str());
        auto r = server->AddService(this,
                                    melon::SERVER_DOESNT_OWN_SERVICE,
                                    map_str);
        if (r != 0) {
            return turbo::internal_error("register restful service failed");
        }
        registered_ = true;
        return turbo::OkStatus();
    }

    mutil::FilePath WebuiService::get_file_meta(const RestfulRequest *request) {
        auto &unresolved_path = request->unresolved_path();
        if (unresolved_path.empty()) {
            return mutil::FilePath(turbo::str_cat(conf_.root_path, "/", conf_.index_path));
        } else {
            return mutil::FilePath(turbo::str_cat(conf_.root_path, "/", unresolved_path));
        }
    }

    std::shared_ptr<std::string> WebuiService::get_content(const mutil::FilePath &path) {
        auto &fpath = path.value();
        std::shared_ptr<std::string> content;
        {
            std::shared_lock lock(file_cache_mutex_);
            auto it = file_cache_.try_get(fpath);
            if(it.second) {
                content = it.first;
            }
        }
        if (content) {
            return content;
        }
        mutil::ScopedFILE fp(fopen(fpath.c_str(), "r"));
        if(!fp) {
            return nullptr;
        }
        fseek(fp.get(), 0, SEEK_END);
        size_t size = ftell(fp.get());
        fseek(fp.get(), 0, SEEK_SET);
        std::string file_content(size, '\0');
        auto r = fread(&file_content[0], 1, size, fp.get());
        if(r != size) {
            return nullptr;
        }
        {
            std::unique_lock lock(file_cache_mutex_);
            file_cache_.put(fpath, file_content);
        }
        return std::make_shared<std::string>(file_content);
    }

    void WebuiService::process_not_found(const RestfulRequest *request, RestfulResponse *response) {
        response->set_status_code(404);
        response->set_content_type("text/html");
        response->set_body(conf_.not_found_str);

        if(conf_.not_found_path.empty()) {
            response->set_body(conf_.not_found_str);
            return;
        }
        auto path = mutil::FilePath(turbo::str_cat(conf_.root_path, "/", conf_.not_found_path));
        auto content = get_content(path);
        if(content) {
            response->set_body(*content);
        } else {
            response->set_body(conf_.not_found_str);
        }
    }
}  // namespace melon
