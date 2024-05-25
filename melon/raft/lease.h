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

#include <melon/raft/util.h>

namespace melon::raft {

    class LeaderLease {
    public:
        enum InternalState {
            DISABLED,
            EXPIRED,
            NOT_READY,
            VALID,
            SUSPECT,
        };

        struct LeaseInfo {
            InternalState state;
            int64_t term;
            int64_t lease_epoch;
        };

        LeaderLease()
                : _election_timeout_ms(0), _last_active_timestamp(0), _term(0), _lease_epoch(0) {}

        void init(int64_t election_timeout_ms);

        void on_leader_start(int64_t term);

        void on_leader_stop();

        void on_lease_start(int64_t expect_lease_epoch, int64_t last_active_timestamp);

        void get_lease_info(LeaseInfo *lease_info);

        void renew(int64_t last_active_timestamp);

        int64_t lease_epoch();

        void reset_election_timeout_ms(int64_t election_timeout_ms);

    private:
        raft_mutex_t _mutex;
        int64_t _election_timeout_ms;
        int64_t _last_active_timestamp;
        int64_t _term;
        int64_t _lease_epoch;
    };

    class FollowerLease {
    public:
        FollowerLease()
                : _election_timeout_ms(0), _max_clock_drift_ms(0), _last_leader_timestamp(0) {}

        void init(int64_t election_timeout_ms, int64_t max_clock_drift_ms);

        void renew(const PeerId &leader_id);

        int64_t votable_time_from_now();

        const PeerId &last_leader();

        bool expired();

        void reset();

        void expire();

        void reset_election_timeout_ms(int64_t election_timeout_ms, int64_t max_clock_drift_ms);

        int64_t last_leader_timestamp();

    private:
        int64_t _election_timeout_ms;
        int64_t _max_clock_drift_ms;
        PeerId _last_leader;
        int64_t _last_leader_timestamp;
    };

} // namespace melon::raft
