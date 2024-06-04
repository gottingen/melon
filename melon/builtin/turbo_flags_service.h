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
#include <melon/builtin/tabbed.h>


namespace melon {

    class TurboFlagsService : public tflags, public Tabbed {
    public:
        void default_method(::google::protobuf::RpcController *cntl_base,
                            const ::melon::TurboFlagsRequest *request,
                            ::melon::TurboFlagsResponse *response,
                            ::google::protobuf::Closure *done);

        void GetTabInfo(TabInfoList *info_list) const;

    private:
        void set_value_page(Controller *cntl, ::google::protobuf::Closure *done);

    };

} // namespace melon
