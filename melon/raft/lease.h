// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#ifndef MELON_RAFT_LEASE_H_
#define MELON_RAFT_LEASE_H_

#include "melon/raft/util.h"

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

#endif // MELON_RAFT_LEASE_H_
