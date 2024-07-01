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

#include <cstdint>
#include <memory>
#include <melon/utility/synchronization/lock.h>
#include <melon/utility/strings/string_piece.h>
#include <melon/utility/strings/string_number_conversions.h>

namespace melon {

    struct ServerId;

    // After all servers are down and health check happens, servers are
    // online one by one. Once one server is up, all the request that should
    // be sent to all servers, would be sent to one server, which may be a
    // disastrous behaviour. In the worst case it would cause the server being down
    // again if circuit breaker is enabled and the cluster would never recover.
    // This class controls the amount of requests that sent to the revived
    // servers when recovering from all servers are down.
    class ClusterRecoverPolicy {
    public:
        virtual ~ClusterRecoverPolicy() {}

        // Indicate that recover from all server being down is happening.
        virtual void StartRecover() = 0;

        // Return true if some customized policies are satisfied.
        virtual bool DoReject(const std::vector<ServerId> &server_list) = 0;

        // Stop recover state and do not reject the request if some condition is
        // satisfied. Return true if the current state is still in recovering.
        virtual bool StopRecoverIfNecessary() = 0;
    };

    // The default cluster recover policy. Once no servers are available, recover is start.
    // If in recover state, the probability that a request is accepted is q/n, in
    // which q is the number of current available server, n is the number of minimum
    // working instances setting by user. If q is not changed during a given time,
    // hold_seconds, then the cluster is considered recovered and all the request
    // would be sent to the current available servers.
    class DefaultClusterRecoverPolicy : public ClusterRecoverPolicy {
    public:
        DefaultClusterRecoverPolicy(int64_t min_working_instances, int64_t hold_seconds);

        void StartRecover() override;

        bool DoReject(const std::vector<ServerId> &server_list) override;

        bool StopRecoverIfNecessary() override;

    private:
        uint64_t GetUsableServerCount(int64_t now_ms, const std::vector<ServerId> &server_list);

    private:
        bool _recovering;
        int64_t _min_working_instances;
        mutil::Mutex _mutex;
        uint64_t _last_usable;
        int64_t _last_usable_change_time_ms;
        int64_t _hold_seconds;
        uint64_t _usable_cache;
        int64_t _usable_cache_time_ms;
    };

    // Return a DefaultClusterRecoverPolicy object by params.
    bool GetRecoverPolicyByParams(const std::string_view &params,
                                  std::shared_ptr<ClusterRecoverPolicy> *ptr_out);

} // namespace melon

