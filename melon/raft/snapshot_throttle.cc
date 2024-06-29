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


#include <melon/utility/time.h>
#include <turbo/flags/flag.h>
#include <melon/raft/snapshot_throttle.h>
#include <melon/raft/util.h>

// used to increase throttle threshold dynamically when user-defined
// threshold is too small in extreme cases.
// notice that this flag does not distinguish disk types(sata or ssd, and so on)
TURBO_FLAG(int64_t, raft_minimal_throttle_threshold_mb, 0,
           "minimal throttle throughput threshold per second").on_validate(
        turbo::GeValidator<int64_t, 0>::validate);

TURBO_FLAG(int32_t, raft_max_install_snapshot_tasks_num, 1000,
           "Max num of install_snapshot tasks per disk at the same time").on_validate(
        turbo::GtValidator<int32_t, 0>::validate);

namespace melon::raft {


    ThroughputSnapshotThrottle::ThroughputSnapshotThrottle(
            int64_t throttle_throughput_bytes, int64_t check_cycle)
            : _throttle_throughput_bytes(throttle_throughput_bytes), _check_cycle(check_cycle), _snapshot_task_num(0),
              _last_throughput_check_time_us(
                      caculate_check_time_us(mutil::cpuwide_time_us(), check_cycle)), _cur_throughput_bytes(0) {}

    ThroughputSnapshotThrottle::~ThroughputSnapshotThrottle() {}

    size_t ThroughputSnapshotThrottle::throttled_by_throughput(int64_t bytes) {
        size_t available_size = bytes;
        int64_t now = mutil::cpuwide_time_us();
        int64_t limit_throughput_bytes_s = std::max(_throttle_throughput_bytes,
                                                    turbo::get_flag(FLAGS_raft_minimal_throttle_threshold_mb) * 1024 *
                                                    1024);
        int64_t limit_per_cycle = limit_throughput_bytes_s / _check_cycle;
        std::unique_lock<raft_mutex_t> lck(_mutex);
        if (_cur_throughput_bytes + bytes > limit_per_cycle) {
            // reading another |bytes| excceds the limit
            if (now - _last_throughput_check_time_us <=
                1 * 1000 * 1000 / _check_cycle) {
                // if time interval is less than or equal to a cycle, read more data
                // to make full use of the throughput of current cycle.
                available_size = limit_per_cycle > _cur_throughput_bytes ? limit_per_cycle - _cur_throughput_bytes : 0;
                _cur_throughput_bytes = limit_per_cycle;
            } else {
                // otherwise, read the data in the next cycle.
                available_size = bytes > limit_per_cycle ? limit_per_cycle : bytes;
                _cur_throughput_bytes = available_size;
                _last_throughput_check_time_us =
                        caculate_check_time_us(now, _check_cycle);
            }
        } else {
            // reading another |bytes| doesn't excced limit(less than or equal to),
            // put it in current cycle
            available_size = bytes;
            _cur_throughput_bytes += available_size;
        }
        lck.unlock();
        return available_size;
    }

    bool ThroughputSnapshotThrottle::add_one_more_task(bool is_leader) {
        // Don't throttle leader, let follower do it
        if (is_leader) {
            return true;
        }
        int task_num_threshold = turbo::get_flag(FLAGS_raft_max_install_snapshot_tasks_num);
        std::unique_lock<raft_mutex_t> lck(_mutex);
        int saved_task_num = _snapshot_task_num;
        if (_snapshot_task_num >= task_num_threshold) {
            lck.unlock();
            LOG(WARNING) << "Fail to add one more task when current task num is: "
                         << saved_task_num << ", task num threshold: " << task_num_threshold;
            return false;
        }
        saved_task_num = ++_snapshot_task_num;
        lck.unlock();
        LOG(INFO) << "Succed to add one more task, new task num is: " << saved_task_num
                  << ", task num threshold: " << task_num_threshold;
        return true;
    }

    void ThroughputSnapshotThrottle::finish_one_task(bool is_leader) {
        if (is_leader) {
            return;
        }
        std::unique_lock<raft_mutex_t> lck(_mutex);
        int saved_task_num = --_snapshot_task_num;
        // _snapshot_task_num should not be negative
        CHECK_GE(_snapshot_task_num, 0) << "Finishing task cause wrong task num: "
                                        << saved_task_num;
        lck.unlock();
        LOG(INFO) << "Finish one task, new task num is: " << saved_task_num;
        return;
    }

    void ThroughputSnapshotThrottle::return_unused_throughput(
            int64_t acquired, int64_t consumed, int64_t elaspe_time_us) {
        int64_t now = mutil::cpuwide_time_us();
        std::unique_lock<raft_mutex_t> lck(_mutex);
        if (now - elaspe_time_us < _last_throughput_check_time_us) {
            // Tokens are aqured in last cycle, ignore
            return;
        }
        _cur_throughput_bytes = std::max(
                _cur_throughput_bytes - (acquired - consumed), int64_t(0));
    }

}  //  namespace melon::raft
