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
#include <melon/rpc/builtin.h>
#include <turbo/strings/str_format.h>
#include <melon/rpc/server.h>
#include <turbo/log/logging.h>
#include <melon/utility/file_util.h>
#include <turbo/strings/str_split.h>
#include <turbo/flags/flag.h>

TURBO_FLAG(std::string , melon_builtin_restful_mapping_path, "/eabi", "mapping path for melon builtin restful service");


class NotFoundProcessor : public melon::BuiltinProcessor {
private:
    void process(const melon::RestfulRequest *request, melon::RestfulResponse *response) override {
        auto path = request->unresolved_path();
        response->set_status_code(404);
        response->set_header("Access-Control-Allow-Origin", "*");
        response->set_header("Access-Control-Allow-Method", "*");
        response->set_header("Access-Control-Allow-Headers", "*");
        response->set_header("Access-Control-Allow-Credentials", "true");
        response->set_header("Access-Control-Expose-Headers", "*");
        response->set_header("Content-Type", "text/plain");
        response->set_body("not found\n");
        response->append_body("Request path: ");
        response->append_body(path);
        response->append_body("\n");
    }
};

class RootProcessor : public melon::BuiltinProcessor {
private:
    void process(const melon::RestfulRequest *request, melon::RestfulResponse *response) override {
        auto path = request->unresolved_path();
        response->set_status_code(200);
        response->set_header("Access-Control-Allow-Origin", "*");
        response->set_header("Access-Control-Allow-Method", "*");
        response->set_header("Access-Control-Allow-Headers", "*");
        response->set_header("Access-Control-Allow-Credentials", "true");
        response->set_header("Access-Control-Expose-Headers", "*");
        response->set_header("Content-Type", "text/plain");
        response->set_body("I am  root\n");
        response->append_body("\n");
    }
};


namespace melon {

    static std::string normalize_path(const std::string &path) {
        std::string result;
        std::vector<std::string_view> segments = turbo::str_split(path, "/", turbo::SkipEmpty());
        bool first = true;
        for (const auto &seg : segments) {
            if(first) {
                first = false;
            } else {
                result.push_back('/');
            }
            result.append(seg);
        }
        return result;
    }

    void BuiltinRestful::impl_method(::google::protobuf::RpcController *controller,
                                        const ::melon::NoUseBuiltinRequest *request,
                                        ::melon::NoUseBuiltinResponse *response,
                                        ::google::protobuf::Closure *done) {
        (void) request;
        (void) response;
        melon::ClosureGuard done_guard(done);
        auto *ctrl = dynamic_cast<Controller *>(controller);
        const RestfulRequest req(ctrl);
        RestfulResponse resp(ctrl);
        auto &path = req.unresolved_path();
        if(path.empty()) {
            if(root_processor_) {
                root_processor_->process(&req, &resp);
                return;
            }
            if(any_path_processor_) {
                any_path_processor_->process(&req, &resp);
                return;
            }
            if(not_found_processor_) {
                not_found_processor_->process(&req, &resp);
                return;
            }
            LOG(FATAL)<<"no processor found for path: /";
        }
        auto it = processors_.find(path);
        if(it != processors_.end()) {
            it->second->process(&req, &resp);
            return;
        }
        if(any_path_processor_) {
            any_path_processor_->process(&req, &resp);
            return;
        }
        if(not_found_processor_) {
            not_found_processor_->process(&req, &resp);
            return;
        }
        LOG(FATAL)<<"no processor found for path: "<<path;
    }

    turbo::Status BuiltinRestful::register_server(Server *server) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(registered_) {
            return turbo::internal_error("register_server can only be called once");
        }
        registered_ = true;
        if(server == nullptr) {
            return turbo::invalid_argument_error("server is empty");
        }
        if(mapping_path_.empty()) {
            mapping_path_ = turbo::get_flag(FLAGS_melon_builtin_restful_mapping_path);
        }
        if(not_found_processor_ == nullptr && any_path_processor_ == nullptr) {
            not_found_processor_ = std::make_shared<NotFoundProcessor>();
        }

        if(root_processor_ == nullptr) {
            root_processor_ = std::make_shared<RootProcessor>();
        }

        if(any_path_processor_ == nullptr && processors_.empty()) {
            return turbo::invalid_argument_error("any_path_processor and processors are both empty, you must set one of them");
        }

        std::string map_str = turbo::str_format("%s/* => impl_method", mapping_path_.c_str());
        auto r = server->AddService(this,
                          melon::SERVER_DOESNT_OWN_SERVICE,
                          map_str);
        if(r != 0) {
            return turbo::internal_error("register restful service failed");
        }
        return turbo::OkStatus();
    }

    BuiltinRestful* BuiltinRestful::set_not_found_processor(std::shared_ptr<BuiltinProcessor> processor) {
        std::lock_guard<std::mutex> lock(mutex_);
        LOG_IF(FATAL, registered_)<<"set_not_found_processor must be called before register_server";
        not_found_processor_ = std::move(processor);
        return this;
    }
    BuiltinRestful* BuiltinRestful::set_any_path_processor(std::shared_ptr<BuiltinProcessor> processor) {
        std::lock_guard<std::mutex> lock(mutex_);
        LOG_IF(FATAL, registered_)<<"set_not_found_processor must be called before register_server";
        any_path_processor_ = std::move(processor);
        return this;
    }
    BuiltinRestful* BuiltinRestful::set_root_processor(std::shared_ptr<BuiltinProcessor> processor) {
        std::lock_guard<std::mutex> lock(mutex_);
        LOG_IF(FATAL, registered_)<<"set_not_found_processor must be called before register_server";
        root_processor_ = std::move(processor);
        return this;
    }
    BuiltinRestful* BuiltinRestful::set_processor(const std::string &path, std::shared_ptr<BuiltinProcessor> processor, bool overwrite) {
        std::lock_guard<std::mutex> lock(mutex_);
        LOG_IF(FATAL, registered_)<<"set_not_found_processor must be called before register_server";
        auto fpath = normalize_path(path);
        if(fpath.empty()) {
            LOG(FATAL)<<"path is empty";
        }
        if(!overwrite) {
            auto it = processors_.find(fpath);
            if(it != processors_.end()) {
                LOG(FATAL)<<"processor already exists for path: "<<path;
            }
        }
        processors_[fpath] = std::move(processor);
        return this;
    }

    BuiltinRestful* BuiltinRestful::set_mapping_path(const std::string &mapping_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        LOG_IF(FATAL, registered_)<<"set_not_found_processor must be called before register_server";
        mapping_path_ = mapping_path;
        return this;
    }

}  // namespace melon
