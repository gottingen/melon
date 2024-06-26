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

#include <melon/utility/macros.h>                        // MELON_CACHELINE_ALIGNMENT
#include <melon/utility/containers/flat_map.h>           // mutil::FlatMap
#include <deque>                                // std::deque
#include <melon/fiber/execution_queue.h>            // fiber::ExecutionQueueId

#include <melon/raft/raft.h>                          // Closure
#include <melon/raft/util.h>                          // raft_mutex_t
#include <melon/raft/log_entry.h>                     // LogEntry
#include <melon/raft/configuration_manager.h>         // ConfigurationManager
#include <melon/raft/storage.h>                       // Storage

namespace melon::raft {

    class LogStorage;

    class FSMCaller;

    struct LogManagerOptions {
        LogManagerOptions();

        LogStorage *log_storage;
        ConfigurationManager *configuration_manager;
        FSMCaller *fsm_caller;  // To report log error
    };

    struct LogManagerStatus {
        LogManagerStatus()
                : first_index(1), last_index(0), disk_index(0), known_applied_index(0) {}

        int64_t first_index;
        int64_t last_index;
        int64_t disk_index;
        int64_t known_applied_index;
    };

    class SnapshotMeta;

    class MELON_CACHELINE_ALIGNMENT LogManager {
    public:
        typedef int64_t WaitId;

        class StableClosure : public Closure {
        public:
            StableClosure() : _first_log_index(0) {}

            void update_metric(IOMetric *metric);

        protected:
            int64_t _first_log_index;
            IOMetric metric;
        private:
            friend class LogManager;

            friend class AppendBatcher;

            std::vector<LogEntry *> _entries;
        };

        LogManager();

        BRAFT_MOCK ~LogManager();

        int init(const LogManagerOptions &options);

        void shutdown();

        // Append log entry vector and wait until it's stable (NOT COMMITTED!)
        // success return 0, fail return errno
        void append_entries(std::vector<LogEntry *> *entries, StableClosure *done);

        // Notify the log manager about the latest snapshot, which indicates the
        // logs which can be safely truncated.
        BRAFT_MOCK void set_snapshot(const SnapshotMeta *meta);

        // We don't delete all the logs before last snapshot to avoid installing
        // snapshot on slow replica. Call this method to drop all the logs before
        // last snapshot immediately.
        BRAFT_MOCK void clear_bufferred_logs();

        // Get the log at |index|
        // Returns:
        //  success return ptr, fail return null
        LogEntry *get_entry(const int64_t index);

        // Get the log term at |index|
        // Returns:
        //  success return term > 0, fail return 0
        int64_t get_term(const int64_t index);

        // Get the first log index of log
        // Returns:
        //  success return first log index, empty return 0
        int64_t first_log_index();

        // Get the last log index of log
        // Returns:
        //  success return last memory and logstorage index, empty return 0
        int64_t last_log_index(bool is_flush = false);

        // Return the id the last log.
        LogId last_log_id(bool is_flush = false);

        void get_configuration(int64_t index, ConfigurationEntry *conf);

        // Check if |current| should be updated to the latest configuration
        // Returns true and |current| is assigned to the lastest configuration, returns
        // false otherweise
        bool check_and_set_configuration(ConfigurationEntry *current);

        // Wait until there are more logs since |last_log_index| and |on_new_log|
        // would be called after there are new logs or error occurs
        WaitId wait(int64_t expected_last_log_index,
                    int (*on_new_log)(void *arg, int error_code), void *arg);

        // Remove a waiter
        // Returns:
        //  - 0: success
        //  - -1: id is Invalid
        int remove_waiter(WaitId id);


        // Set the applied id, indicating that the log before applied_id (inclded)
        // can be droped from memory logs
        void set_applied_id(const LogId &applied_id);

        // Check the consistency between log and snapshot, which must satisfy ANY
        // one of the following condition
        //   - Log starts from 1. OR
        //   - Log starts from a positive position and there must be a snapshot
        //     of which the last_included_id is in the range
        //     [first_log_index-1, last_log_index]
        // Returns mutil::Status::OK if valid, a specific error otherwise
        mutil::Status check_consistency();

        void describe(std::ostream &os, bool use_html);

        // Get the internal status of LogManager.
        void get_status(LogManagerStatus *status);

    private:
        friend class AppendBatcher;

        struct WaitMeta {
            int (*on_new_log)(void *arg, int error_code);

            void *arg;
            int error_code;
        };

        void append_to_storage(std::vector<LogEntry *> *to_append, LogId *last_id, IOMetric *metric);

        static int disk_thread(void *meta,
                               fiber::TaskIterator<StableClosure *> &iter);

        // delete logs from storage's head, [1, first_index_kept) will be discarded
        // Returns:
        //  success return 0, failed return -1
        int truncate_prefix(const int64_t first_index_kept,
                            std::unique_lock<raft_mutex_t> &lck);

        int reset(const int64_t next_log_index,
                  std::unique_lock<raft_mutex_t> &lck);

        // Must be called in the disk thread, otherwise the
        // behavior is undefined
        void set_disk_id(const LogId &disk_id);

        LogEntry *get_entry_from_memory(const int64_t index);

        WaitId notify_on_new_log(int64_t expected_last_log_index, WaitMeta *wm);

        int check_and_resolve_conflict(std::vector<LogEntry *> *entries,
                                       StableClosure *done);

        void unsafe_truncate_suffix(const int64_t last_index_kept);

        // Clear the logs in memory whose id <= the given |id|
        void clear_memory_logs(const LogId &id);

        int64_t unsafe_get_term(const int64_t index);

        // Start a independent thread to append log to LogStorage
        int start_disk_thread();

        int stop_disk_thread();

        void wakeup_all_waiter(std::unique_lock<raft_mutex_t> &lck);

        static void *run_on_new_log(void *arg);

        void report_error(int error_code, const char *fmt, ...);

        // Fast implementation with one lock
        // TODO(chenzhangyi01): reduce the critical section
        LogStorage *_log_storage;
        ConfigurationManager *_config_manager;
        FSMCaller *_fsm_caller;

        raft_mutex_t _mutex;
        mutil::FlatMap<int64_t, WaitMeta *> _wait_map;
        bool _stopped;
        mutil::atomic<bool> _has_error;
        WaitId _next_wait_id;

        LogId _disk_id;
        LogId _applied_id;
        // TODO(chenzhangyi01): replace deque with a thread-safe data structure
        std::deque<LogEntry * /*FIXME*/> _logs_in_memory;
        int64_t _first_log_index;
        int64_t _last_log_index;
        // the last snapshot's log_id
        LogId _last_snapshot_id;
        // the virtual first log, for finding next_index of replicator, which
        // can avoid install_snapshot too often in extreme case where a follower's
        // install_snapshot is slower than leader's save_snapshot
        // [NOTICE] there should not be hole between this log_id and _last_snapshot_id,
        // or may cause some unexpect cases
        LogId _virtual_first_log_id;

        fiber::ExecutionQueueId<StableClosure *> _disk_queue;
    };

}  //  namespace melon::raft
