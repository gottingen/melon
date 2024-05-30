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

#include <melon/utility/memory/ref_counted.h>                // mutil::RefCountedThreadSafe
#include <melon/raft/util.h>

namespace melon::raft {

    // Abstract class with the function of throttling during heavy disk reading/writing
    class SnapshotThrottle : public mutil::RefCountedThreadSafe<SnapshotThrottle> {
    public:
        SnapshotThrottle() {}

        virtual ~SnapshotThrottle() {}

        // Get available throughput after throttled
        // Must be thread-safe
        virtual size_t throttled_by_throughput(int64_t bytes) = 0;

        virtual bool add_one_more_task(bool is_leader) = 0;

        virtual void finish_one_task(bool is_leader) = 0;

        virtual int64_t get_retry_interval_ms() = 0;

        // After a throttled request finish, |return_unused_throughput| is called to
        // return back unsed tokens (throghput). Default implementation do nothing.
        // There are two situations we can optimize:
        // case 1: The follower and leader both try to throttle the same request, and
        //         only one of them permit the request. No actual IO and bandwidth consumed,
        //         the acquired tokens are wasted.
        // case 2: We acquired some tokens, but only part of them are used, because of
        //         the file reach the eof, or the file contains holes.
        virtual void return_unused_throughput(
                int64_t acquired, int64_t consumed, int64_t elaspe_time_us) {}

    private:
        DISALLOW_COPY_AND_ASSIGN(SnapshotThrottle);

        friend class mutil::RefCountedThreadSafe<SnapshotThrottle>;
    };

// SnapshotThrottle with throughput threshold used in install_snapshot
    class ThroughputSnapshotThrottle : public SnapshotThrottle {
    public:
        ThroughputSnapshotThrottle(int64_t throttle_throughput_bytes, int64_t check_cycle);

        int64_t get_throughput() const { return _throttle_throughput_bytes; }

        int64_t get_cycle() const { return _check_cycle; }

        size_t throttled_by_throughput(int64_t bytes);

        bool add_one_more_task(bool is_leader);

        void finish_one_task(bool is_leader);

        int64_t get_retry_interval_ms() { return 1000 / _check_cycle + 1; }

        void return_unused_throughput(
                int64_t acquired, int64_t consumed, int64_t elaspe_time_us);

    private:
        ~ThroughputSnapshotThrottle();

        // user defined throughput threshold for raft, bytes per second
        int64_t _throttle_throughput_bytes;
        // user defined check cycles of throughput per second
        int64_t _check_cycle;
        // the num of tasks doing install_snapshot
        int _snapshot_task_num;
        int64_t _last_throughput_check_time_us;
        int64_t _cur_throughput_bytes;
        raft_mutex_t _mutex;
    };

    inline int64_t caculate_check_time_us(int64_t current_time_us,
                                          int64_t check_cycle) {
        int64_t base_aligning_time_us = 1000 * 1000 / check_cycle;
        return current_time_us / base_aligning_time_us * base_aligning_time_us;
    }

} //  namespace melon::raft
