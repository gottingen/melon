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



#include <melon/rpc/server_id.h>


namespace melon {

ServerId2SocketIdMapper::ServerId2SocketIdMapper() {
    _tmp.reserve(128);
    MCHECK_EQ(0, _nref_map.init(128));
}

ServerId2SocketIdMapper::~ServerId2SocketIdMapper() {
}

bool ServerId2SocketIdMapper::AddServer(const ServerId& server) {
    return (++_nref_map[server.id] == 1);
}

bool ServerId2SocketIdMapper::RemoveServer(const ServerId& server) {
    int* nref = _nref_map.seek(server.id);
    if (nref == NULL) {
        MLOG(ERROR) << "Unexist SocketId=" << server.id;
        return false;
    }
    if (--*nref <= 0) {
        _nref_map.erase(server.id);
        return true;
    }
    return false;
}

std::vector<SocketId>& ServerId2SocketIdMapper::AddServers(
    const std::vector<ServerId>& servers) {
    _tmp.clear();
    for (size_t i = 0; i < servers.size(); ++i) {
        if (AddServer(servers[i])) {
            _tmp.push_back(servers[i].id);
        }
    }
    return _tmp;
}

std::vector<SocketId>& ServerId2SocketIdMapper::RemoveServers(
    const std::vector<ServerId>& servers) {
    _tmp.clear();
    for (size_t i = 0; i < servers.size(); ++i) {
        if (RemoveServer(servers[i])) {
            _tmp.push_back(servers[i].id);
        }
    }
    return _tmp;
}

} // namespace melon
