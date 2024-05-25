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

#include <melon/proto/rpc/get_js.pb.h>


namespace melon {

    // Get packed js.
    //   "/js/sorttable"  : http://www.kryogenix.org/code/browser/sorttable/
    //   "/js/jquery_min" : jquery 1.8.3
    //   "/js/flot_min"   : plotting library for jquery.
    class GetJsService : public ::melon::js {
    public:
        void sorttable(::google::protobuf::RpcController *controller,
                       const GetJsRequest *request,
                       GetJsResponse *response,
                       ::google::protobuf::Closure *done) override;

        void jquery_min(::google::protobuf::RpcController *controller,
                        const GetJsRequest *request,
                        GetJsResponse *response,
                        ::google::protobuf::Closure *done) override;

        void flot_min(::google::protobuf::RpcController *controller,
                      const GetJsRequest *request,
                      GetJsResponse *response,
                      ::google::protobuf::Closure *done) override;

        void viz_min(::google::protobuf::RpcController *controller,
                     const GetJsRequest *request,
                     GetJsResponse *response,
                     ::google::protobuf::Closure *done) override;
    };

} // namespace melon
