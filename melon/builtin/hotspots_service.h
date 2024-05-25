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

#include <melon/builtin/common.h>
#include <melon/proto/rpc/builtin_service.pb.h>
#include <melon/builtin/tabbed.h>


namespace melon {

    class Server;

    class HotspotsService : public hotspots, public Tabbed {
    public:
        void cpu(::google::protobuf::RpcController *cntl_base,
                 const ::melon::HotspotsRequest *request,
                 ::melon::HotspotsResponse *response,
                 ::google::protobuf::Closure *done) override;

        void heap(::google::protobuf::RpcController *cntl_base,
                  const ::melon::HotspotsRequest *request,
                  ::melon::HotspotsResponse *response,
                  ::google::protobuf::Closure *done) override;

        void growth(::google::protobuf::RpcController *cntl_base,
                    const ::melon::HotspotsRequest *request,
                    ::melon::HotspotsResponse *response,
                    ::google::protobuf::Closure *done) override;

        void contention(::google::protobuf::RpcController *cntl_base,
                        const ::melon::HotspotsRequest *request,
                        ::melon::HotspotsResponse *response,
                        ::google::protobuf::Closure *done) override;

        void cpu_non_responsive(::google::protobuf::RpcController *cntl_base,
                                const ::melon::HotspotsRequest *request,
                                ::melon::HotspotsResponse *response,
                                ::google::protobuf::Closure *done) override;

        void heap_non_responsive(::google::protobuf::RpcController *cntl_base,
                                 const ::melon::HotspotsRequest *request,
                                 ::melon::HotspotsResponse *response,
                                 ::google::protobuf::Closure *done) override;

        void growth_non_responsive(::google::protobuf::RpcController *cntl_base,
                                   const ::melon::HotspotsRequest *request,
                                   ::melon::HotspotsResponse *response,
                                   ::google::protobuf::Closure *done) override;

        void contention_non_responsive(::google::protobuf::RpcController *cntl_base,
                                       const ::melon::HotspotsRequest *request,
                                       ::melon::HotspotsResponse *response,
                                       ::google::protobuf::Closure *done) override;

        void GetTabInfo(melon::TabInfoList *) const override;
    };

} // namespace melon
