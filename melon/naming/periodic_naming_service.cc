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



#include <gflags/gflags.h>
#include <melon/utility/logging.h>
#include <melon/fiber/fiber.h>
#include <melon/rpc/log.h>
#include <melon/rpc/reloadable_flags.h>
#include <melon/naming/periodic_naming_service.h>

namespace melon {

    DEFINE_int32(ns_access_interval, 5,
                 "Wait so many seconds before next access to naming service");
    MELON_VALIDATE_GFLAG(ns_access_interval, PositiveInteger);

    int PeriodicNamingService::GetNamingServiceAccessIntervalMs() const {
        return std::max(FLAGS_ns_access_interval, 1) * 1000;
    }

    int PeriodicNamingService::RunNamingService(
            const char *service_name, NamingServiceActions *actions) {
        std::vector<ServerNode> servers;
        bool ever_reset = false;
        while (true) {
            servers.clear();
            const int rc = GetServers(service_name, &servers);
            if (rc == 0) {
                ever_reset = true;
                actions->ResetServers(servers);
            } else if (!ever_reset) {
                // ResetServers must be called at first time even if GetServers
                // failed, to wake up callers to `WaitForFirstBatchOfServers'
                ever_reset = true;
                servers.clear();
                actions->ResetServers(servers);
            }

            // If `fiber_stop' is called to stop the ns fiber when `melon::Join‘ is called
            // in `GetServers' to wait for a rpc to complete. The fiber will be woken up,
            // reset `TaskMeta::interrupted' and continue to join the rpc. After the rpc is complete,
            // `fiber_usleep' will not sense the interrupt signal and sleep successfully.
            // Finally, the ns fiber will never exit. So need to check the stop status of
            // the fiber here and exit the fiber in time.
            if (fiber_stopped(fiber_self())) {
                RPC_VMLOG << "Quit NamingServiceThread=" << fiber_self();
                return 0;
            }
            if (fiber_usleep(GetNamingServiceAccessIntervalMs() * 1000UL) < 0) {
                if (errno == ESTOP) {
                    RPC_VMLOG << "Quit NamingServiceThread=" << fiber_self();
                    return 0;
                }
                PMLOG(FATAL) << "Fail to sleep";
                return -1;
            }
        }
    }

} // namespace melon
