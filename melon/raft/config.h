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

#pragma once

#include <turbo/flags/declare.h>

    // file service check hole switch, default disable
    TURBO_DECLARE_FLAG(bool, raft_file_check_hole);

    // Max numbers of logs for the state machine to commit in a single batch
    //  Default: 512
    TURBO_DECLARE_FLAG(int32_t, raft_fsm_caller_commit_batch);
    
    // Use fsync rather than fdatasync to flush page cache
    // Default: true
    TURBO_DECLARE_FLAG(bool, raft_use_fsync_rather_than_fdatasync);
    
    // Enable or disable leader lease. only when all peers in a raft group
    // set this configuration to true, leader lease check and vote are safe.
    // Default: false
    TURBO_DECLARE_FLAG(bool, raft_enable_leader_lease);
    
    // Max size of one segment file
    // Default: 8M
    TURBO_DECLARE_FLAG(int32_t, raft_max_segment_size);

    // call fsync when a segment is closed
    // Default: false
    TURBO_DECLARE_FLAG(bool, raft_sync_segments);

    // Max leader io batch
    // Default: 256
    TURBO_DECLARE_FLAG(int32_t, raft_leader_batch);

    // Max election delay time allowed by user
    // Default: 1000
    TURBO_DECLARE_FLAG(int32_t, raft_max_election_delay_ms);

    // candidate steps down when reaching timeout
    // Default: true
    TURBO_DECLARE_FLAG(bool, raft_step_down_when_vote_timedout);

    // enable cache for out-of-order append entries requests, should used when
    // pipeline replication is enabled (raft_max_parallel_append_entries_rpc_num > 1).
    // Default: false
    TURBO_DECLARE_FLAG(bool, raft_enable_append_entries_cache);

    // the max size of out-of-order append entries cache
    // Default: 8
    TURBO_DECLARE_FLAG(int32_t, raft_max_append_entries_cache_size);

    // append entry low latency us
    // Default: 1000 * 1000 (1s)
    TURBO_DECLARE_FLAG(int64_t, raft_append_entry_high_lat_us);

    // trace append entry latency
    // Default: false
    TURBO_DECLARE_FLAG(bool, raft_trace_append_entry_latency);

    // timeout in milliseconds for establishing connections of RPCs
    // Default: 200
    TURBO_DECLARE_FLAG(int32_t, raft_rpc_channel_connect_timeout_ms);

    // enable witness temporarily to become leader when leader down accidently
    // Default: false
    TURBO_DECLARE_FLAG(bool, raft_enable_witness_to_leader);

    // max number of tasks that can be written into db in a single batch
    // Default: 128
    TURBO_DECLARE_FLAG(int32_t, raft_meta_write_batch);

    // Maximum of block size per RPC
    // Default: 128K (1024 * 128)
    TURBO_DECLARE_FLAG(int32_t, raft_max_byte_count_per_rpc);

    // Whether allowing read snapshot data partly
    // Default: true
    TURBO_DECLARE_FLAG(bool, raft_allow_read_partly_when_install_snapshot);

    // enable throttle when install snapshot, for both leader and follower
    // Default: true
    TURBO_DECLARE_FLAG(bool, raft_enable_throttle_when_install_snapshot);
    
    // The max number of entries in AppendEntriesRequest
    // Default: 1024
    TURBO_DECLARE_FLAG(int32_t, raft_max_entries_size);

    // The max number of parallel AppendEntries requests
    // Default: 1
    TURBO_DECLARE_FLAG(int32_t, raft_max_parallel_append_entries_rpc_num);

    // The max byte size of AppendEntriesRequest
    // Default: 512K (1024 * 512)
    TURBO_DECLARE_FLAG(int32_t, raft_max_body_size);

    // Interval of retry to append entries or install snapshot
    // Default: 1000
    TURBO_DECLARE_FLAG(int32_t, raft_retry_replicate_interval_ms);

    // Initial capacity of RouteTable
    // Default: 128
    TURBO_DECLARE_FLAG(int32_t, initial_route_table_cap);

    // Will do snapshot only when actual gap between applied_index and
    // last_snapshot_index is equal to or larger than this value
    // Default: 1
    TURBO_DECLARE_FLAG(int32_t, raft_do_snapshot_min_index_gap);

    // minimal throttle throughput threshold per second
    // Default: 0
    TURBO_DECLARE_FLAG(int64_t, raft_minimal_throttle_threshold_mb);

    // Max num of install_snapshot tasks per disk at the same time
    // Default: 1000
    TURBO_DECLARE_FLAG(int32_t, raft_max_install_snapshot_tasks_num);

    // call fsync when need
    // Default: true
    TURBO_DECLARE_FLAG(bool, raft_sync);

    // sync raft log per bytes when raft_sync set to true
    // Default: INT32_MAX
    TURBO_DECLARE_FLAG(int32_t, raft_sync_per_bytes);

    // Create parent directories of the path in local storage if true
    // Default: true
    TURBO_DECLARE_FLAG(bool, raft_create_parent_directories);

    // raft sync policy when raft_sync set to true, 0 mean sync immediately, 1
    // mean sync by write bytes
    // Default: 0
    TURBO_DECLARE_FLAG(int32_t, raft_sync_policy);

    // sync log meta, snapshot meta and raft meta
    // Default: false
    TURBO_DECLARE_FLAG(bool, raft_sync_meta);

    // First counter percentile
    // Default: 80
    TURBO_DECLARE_FLAG(int32_t, var_counter_p1);

    // Second counter percentile
    // Default: 90
    TURBO_DECLARE_FLAG(int32_t, var_counter_p2);

    // Third counter percentile
    // Default: 99
    TURBO_DECLARE_FLAG(int32_t, var_counter_p3);

