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

#include <melon/proto/rpc/builtin_service.pb.h>


namespace melon {

    class PProfService : public pprof {
    public:
        void profile(::google::protobuf::RpcController *controller,
                     const ::melon::ProfileRequest *request,
                     ::melon::ProfileResponse *response,
                     ::google::protobuf::Closure *done);

        void contention(::google::protobuf::RpcController *controller,
                        const ::melon::ProfileRequest *request,
                        ::melon::ProfileResponse *response,
                        ::google::protobuf::Closure *done);

        void heap(::google::protobuf::RpcController *controller,
                  const ::melon::ProfileRequest *request,
                  ::melon::ProfileResponse *response,
                  ::google::protobuf::Closure *done);

        void growth(::google::protobuf::RpcController *controller,
                    const ::melon::ProfileRequest *request,
                    ::melon::ProfileResponse *response,
                    ::google::protobuf::Closure *done);

        void symbol(::google::protobuf::RpcController *controller,
                    const ::melon::ProfileRequest *request,
                    ::melon::ProfileResponse *response,
                    ::google::protobuf::Closure *done);

        void cmdline(::google::protobuf::RpcController *controller,
                     const ::melon::ProfileRequest *request,
                     ::melon::ProfileResponse *response,
                     ::google::protobuf::Closure *done);
    };

} // namespace melon
