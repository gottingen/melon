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

#include <melon/proto/rpc/restful.pb.h>
#include <melon/rpc/restful_request.h>
#include <melon/rpc/restful_response.h>
#include <turbo/utility/status.h>
#include <melon/rpc/channel.h>
#include <memory>

namespace melon {

    class RestfulService : public melon::restful {
    public:
        RestfulService() = default;

        void impl_method(::google::protobuf::RpcController *controller,
                         const ::melon::NoUseRestfulRequest *request,
                         ::melon::NoUseRestfulResponse *response,
                         ::google::protobuf::Closure *done) override;

        turbo::Status register_server(const std::string &mapping_path, Server *server);

    protected:
        virtual void process(const RestfulRequest *request, RestfulResponse *response) = 0;

    private:
        std::string mapping_path_;
    };

    class RestfulClient {
    public:
        RestfulClient() = default;

        void set_channel(turbo::Nonnull<Channel *> channel) {
            channel_ = channel;
        }

        turbo::Result<RestfulRequest> create_request() {
            if(channel_ == nullptr) {
                return turbo::internal_error("channel is nullptr");
            }

            if(in_use_) {
                return turbo::internal_error("RestfulClient is in use");
            }
            controller_.Reset();
            in_use_ = true;
            return RestfulRequest(&controller_);
        }

        RestfulResponse do_request(const RestfulRequest &request) {
            RestfulResponse response(&controller_);
            channel_->CallMethod(nullptr, &controller_, nullptr, nullptr, nullptr);
            return response;
        }

        void reset() {
            in_use_ = false;
            controller_.Reset();
        }

    private:
        bool in_use_{false};
        Controller controller_;
        Channel *channel_{nullptr};
    };
}  // namespace melon