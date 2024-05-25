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

#include <ostream>
#include <melon/proto/rpc/builtin_service.pb.h>


namespace melon {

    class Server;

    class VersionService : public version {
    public:
        explicit VersionService(Server *server) : _server(server) {}

        void default_method(::google::protobuf::RpcController *cntl_base,
                            const ::melon::VersionRequest *request,
                            ::melon::VersionResponse *response,
                            ::google::protobuf::Closure *done);

    private:
        Server *_server;
    };

} // namespace melon
