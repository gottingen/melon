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


#include <turbo/flags/flag.h>
#include <melon/raft/lease.h>

TURBO_FLAG(bool, raft_enable_leader_lease, false,
           "Enable or disable leader lease. only when all peers in a raft group "
           "set this configuration to true, leader lease check and vote are safe.").on_validate(
        turbo::AllPassValidator<bool>::validate);
namespace melon::raft {

    void LeaderLease::init(int64_t election_timeout_ms) {
        _election_timeout_ms = election_timeout_ms;
    }

    void LeaderLease::on_leader_start(int64_t term) {
        MELON_SCOPED_LOCK(_mutex);
        ++_lease_epoch;
        _term = term;
        _last_active_timestamp = 0;
    }

    void LeaderLease::on_leader_stop() {
        MELON_SCOPED_LOCK(_mutex);
        _last_active_timestamp = 0;
        _term = 0;
    }

    void LeaderLease::on_lease_start(int64_t expect_lease_epoch, int64_t last_active_timestamp) {
        MELON_SCOPED_LOCK(_mutex);
        if (_term == 0 || expect_lease_epoch != _lease_epoch) {
            return;
        }
        _last_active_timestamp = last_active_timestamp;
    }

    void LeaderLease::renew(int64_t last_active_timestamp) {
        MELON_SCOPED_LOCK(_mutex);
        _last_active_timestamp = last_active_timestamp;
    }

    void LeaderLease::get_lease_info(LeaseInfo *lease_info) {
        lease_info->term = 0;
        lease_info->lease_epoch = 0;
        if (!turbo::get_flag(FLAGS_raft_enable_leader_lease)) {
            lease_info->state = LeaderLease::DISABLED;
            return;
        }

        MELON_SCOPED_LOCK(_mutex);
        if (_term == 0) {
            lease_info->state = LeaderLease::EXPIRED;
            return;
        }
        if (_last_active_timestamp == 0) {
            lease_info->state = LeaderLease::NOT_READY;
            return;
        }
        if (mutil::monotonic_time_ms() < _last_active_timestamp + _election_timeout_ms) {
            lease_info->term = _term;
            lease_info->lease_epoch = _lease_epoch;
            lease_info->state = LeaderLease::VALID;
        } else {
            lease_info->state = LeaderLease::SUSPECT;
        }
    }

    int64_t LeaderLease::lease_epoch() {
        MELON_SCOPED_LOCK(_mutex);
        return _lease_epoch;
    }

    void LeaderLease::reset_election_timeout_ms(int64_t election_timeout_ms) {
        MELON_SCOPED_LOCK(_mutex);
        _election_timeout_ms = election_timeout_ms;
    }

    void FollowerLease::init(int64_t election_timeout_ms, int64_t max_clock_drift_ms) {
        _election_timeout_ms = election_timeout_ms;
        _max_clock_drift_ms = max_clock_drift_ms;
        // When the node restart, we are not sure when the lease will be expired actually,
        // so just be conservative.
        _last_leader_timestamp = mutil::monotonic_time_ms();
    }

    void FollowerLease::renew(const PeerId &leader_id) {
        _last_leader = leader_id;
        _last_leader_timestamp = mutil::monotonic_time_ms();
    }

    int64_t FollowerLease::last_leader_timestamp() {
        return _last_leader_timestamp;
    }

    int64_t FollowerLease::votable_time_from_now() {
        if (!turbo::get_flag(FLAGS_raft_enable_leader_lease)) {
            return 0;
        }

        int64_t now = mutil::monotonic_time_ms();
        int64_t votable_timestamp = _last_leader_timestamp + _election_timeout_ms +
                                    _max_clock_drift_ms;
        if (now >= votable_timestamp) {
            return 0;
        }
        return votable_timestamp - now;
    }

    const PeerId &FollowerLease::last_leader() {
        return _last_leader;
    }

    bool FollowerLease::expired() {
        return mutil::monotonic_time_ms() - _last_leader_timestamp
               >= _election_timeout_ms + _max_clock_drift_ms;
    }

    void FollowerLease::reset() {
        _last_leader = PeerId();
        _last_leader_timestamp = 0;
    }

    void FollowerLease::expire() {
        _last_leader_timestamp = 0;
    }

    void FollowerLease::reset_election_timeout_ms(int64_t election_timeout_ms,
                                                  int64_t max_clock_drift_ms) {
        _election_timeout_ms = election_timeout_ms;
        _max_clock_drift_ms = max_clock_drift_ms;
    }

} // namespace melon::raft
