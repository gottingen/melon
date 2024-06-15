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

#include <melon/base/intrusive_ptr.h>
#include <melon/rpc/load_balancer.h>
#include <melon/naming/naming_service_thread.h>         // NamingServiceWatcher


namespace melon {

class LoadBalancerWithNaming : public SharedLoadBalancer,
                               public NamingServiceWatcher {
public:
    LoadBalancerWithNaming() {}
    ~LoadBalancerWithNaming();

    int Init(const char* ns_url, const char* lb_name,
             const NamingServiceFilter* filter,
             const GetNamingServiceThreadOptions* options);
    
    void OnAddedServers(const std::vector<ServerId>& servers);
    void OnRemovedServers(const std::vector<ServerId>& servers);

    void Describe(std::ostream& os, const DescribeOptions& options);

private:
    mutil::intrusive_ptr<NamingServiceThread> _nsthread_ptr;
};

} // namespace melon
