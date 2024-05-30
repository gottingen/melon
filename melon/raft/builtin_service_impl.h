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

#include <melon/proto/raft/builtin_service.pb.h>
#include <melon/builtin/tabbed.h>

namespace melon::raft {

    class RaftStatImpl : public raft_stat, public melon::Tabbed {
    public:
        void default_method(::google::protobuf::RpcController *controller,
                            const ::melon::raft::IndexRequest *request,
                            ::melon::raft::IndexResponse *response,
                            ::google::protobuf::Closure *done);

        void GetTabInfo(melon::TabInfoList *) const;
    };

}  //  namespace melon::raft
