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

#include <vector>                                   // std::vector
#include <string>                                   // std::string
#include <ostream>                                  // std::ostream
#include <melon/base/endpoint.h>                         // mutil::EndPoint
#include <melon/base/macros.h>                           // MELON_CONCAT
#include <melon/rpc/describable.h>
#include <melon/rpc/destroyable.h>
#include <melon/rpc/extension.h>                         // Extension<T>
#include <melon/rpc/server_node.h>                       // ServerNode

namespace melon {

    // Continuing actions to added/removed servers.
    // NOTE: You don't have to implement this class.
    class NamingServiceActions {
    public:
        virtual ~NamingServiceActions() {}

        virtual void AddServers(const std::vector<ServerNode> &servers) = 0;

        virtual void RemoveServers(const std::vector<ServerNode> &servers) = 0;

        virtual void ResetServers(const std::vector<ServerNode> &servers) = 0;
    };

    // Mapping a name to ServerNodes.
    class NamingService : public Describable, public Destroyable {
    public:
        // Implement this method to get servers associated with `service_name'
        // in periodic or event-driven manner, call methods of `actions' to
        // tell RPC system about server changes. This method will be run in
        // a dedicated fiber without access from other threads, thus the
        // implementation does NOT need to be thread-safe.
        // Return 0 on success, error code otherwise.
        virtual int RunNamingService(const char *service_name,
                                     NamingServiceActions *actions) = 0;

        // If this method returns true, RunNamingService will be called without
        // a dedicated fiber. As the name implies, this is suitable for static
        // and simple impl, saving the cost of creating a fiber. However most
        // impl of RunNamingService never quit, thread is a must to prevent the
        // method from blocking the caller.
        virtual bool RunNamingServiceReturnsQuickly() { return false; }

        // Create/destroy an instance.
        // Caller is responsible for Destroy() the instance after usage.
        virtual NamingService *New() const = 0;

    protected:
        virtual ~NamingService() {}
    };

    inline Extension<const NamingService> *NamingServiceExtension() {
        return Extension<const NamingService>::instance();
    }

} // namespace melon
