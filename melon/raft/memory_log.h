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

#include <vector>
#include <deque>
#include <atomic>
#include <melon/base/iobuf.h>
#include <turbo/log/logging.h>
#include <melon/raft/log_entry.h>
#include <melon/raft/storage.h>
#include <melon/raft/util.h>

namespace melon::raft {

    class MELON_CACHELINE_ALIGNMENT MemoryLogStorage : public LogStorage {
    public:
        typedef std::deque<LogEntry *> MemoryData;

        MemoryLogStorage(const std::string &path)
                : _path(path),
                  _first_log_index(1),
                  _last_log_index(0) {}

        MemoryLogStorage()
                : _first_log_index(1),
                  _last_log_index(0) {}

        virtual ~MemoryLogStorage() { reset(1); }

        virtual int init(ConfigurationManager *configuration_manager);

        // first log index in log
        virtual int64_t first_log_index() {
            return _first_log_index.load(std::memory_order_acquire);
        }

        // last log index in log
        virtual int64_t last_log_index() {
            return _last_log_index.load(std::memory_order_acquire);
        }

        // get logentry by index
        virtual LogEntry *get_entry(const int64_t index);

        // get logentry's term by index
        virtual int64_t get_term(const int64_t index);

        // append entries to log
        virtual int append_entry(const LogEntry *entry);

        // append entries to log and update IOMetric, return append success number
        virtual int append_entries(const std::vector<LogEntry *> &entries, IOMetric *metric);

        // delete logs from storage's head, [first_log_index, first_index_kept) will be discarded
        virtual int truncate_prefix(const int64_t first_index_kept);

        // delete uncommitted logs from storage's tail, (last_index_kept, last_log_index] will be discarded
        virtual int truncate_suffix(const int64_t last_index_kept);

        // Drop all the existing logs and reset next log index to |next_log_index|.
        // This function is called after installing snapshot from leader
        virtual int reset(const int64_t next_log_index);

        // Create an instance of this kind of LogStorage with the parameters encoded
        // in |uri|
        // Return the address referenced to the instance on success, NULL otherwise.
        virtual LogStorage *new_instance(const std::string &uri) const;

        // GC an instance of this kind of LogStorage with the parameters encoded
        // in |uri|
        virtual mutil::Status gc_instance(const std::string &uri) const;

    private:
        std::string _path;
        std::atomic<int64_t> _first_log_index;
        std::atomic<int64_t> _last_log_index;
        MemoryData _log_entry_data;
        raft_mutex_t _mutex;
    };

} //  namespace melon::raft

