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


#pragma once

#include <vector>
#include <deque>
#include <melon/utility/atomicops.h>
#include <melon/utility/iobuf.h>
#include <melon/utility/logging.h>
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
            return _first_log_index.load(mutil::memory_order_acquire);
        }

        // last log index in log
        virtual int64_t last_log_index() {
            return _last_log_index.load(mutil::memory_order_acquire);
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
        mutil::atomic<int64_t> _first_log_index;
        mutil::atomic<int64_t> _last_log_index;
        MemoryData _log_entry_data;
        raft_mutex_t _mutex;
    };

} //  namespace melon::raft

