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



#include <melon/rpc/details/load_balancer_with_naming.h>


namespace melon {

LoadBalancerWithNaming::~LoadBalancerWithNaming() {
    if (_nsthread_ptr.get()) {
        _nsthread_ptr->RemoveWatcher(this);
    }
}

int LoadBalancerWithNaming::Init(const char* ns_url, const char* lb_name,
                                 const NamingServiceFilter* filter,
                                 const GetNamingServiceThreadOptions* options) {
    if (SharedLoadBalancer::Init(lb_name) != 0) {
        return -1;
    }
    if (GetNamingServiceThread(&_nsthread_ptr, ns_url, options) != 0) {
        MLOG(ERROR) << "Fail to get NamingServiceThread";
        return -1;
    }
    if (_nsthread_ptr->AddWatcher(this, filter) != 0) {
        MLOG(ERROR) << "Fail to add watcher into _server_list";
        return -1;
    }
    return 0;
}

void LoadBalancerWithNaming::OnAddedServers(
    const std::vector<ServerId>& servers) {
    AddServersInBatch(servers);
}

void LoadBalancerWithNaming::OnRemovedServers(
    const std::vector<ServerId>& servers) {
    RemoveServersInBatch(servers);
}

void LoadBalancerWithNaming::Describe(std::ostream& os,
                                      const DescribeOptions& options) {
    if (_nsthread_ptr) {
        _nsthread_ptr->Describe(os, options);
    } else {
        os << "NULL";
    }
    os << " lb=";
    SharedLoadBalancer::Describe(os, options);
}

} // namespace melon
