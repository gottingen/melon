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
#include <map>
#include <melon/utility/memory/ref_counted.h>
#include <melon/utility/atomicops.h>
#include <melon/utility/iobuf.h>
#include <turbo/log/logging.h>
#include <melon/raft/log_entry.h>
#include <melon/raft/storage.h>
#include <melon/raft/util.h>

namespace melon::raft {

    class MELON_CACHELINE_ALIGNMENT Segment
            : public mutil::RefCountedThreadSafe<Segment> {
    public:
        Segment(const std::string &path, const int64_t first_index, int checksum_type)
                : _path(path), _bytes(0), _unsynced_bytes(0),
                  _fd(-1), _is_open(true),
                  _first_index(first_index), _last_index(first_index - 1),
                  _checksum_type(checksum_type) {}

        Segment(const std::string &path, const int64_t first_index, const int64_t last_index,
                int checksum_type)
                : _path(path), _bytes(0), _unsynced_bytes(0),
                  _fd(-1), _is_open(false),
                  _first_index(first_index), _last_index(last_index),
                  _checksum_type(checksum_type) {}

        struct EntryHeader;

        // create open segment
        int create();

        // load open or closed segment
        // open fd, load index, truncate uncompleted entry
        int load(ConfigurationManager *configuration_manager);

        // serialize entry, and append to open segment
        int append(const LogEntry *entry);

        // get entry by index
        LogEntry *get(const int64_t index) const;

        // get entry's term by index
        int64_t get_term(const int64_t index) const;

        // close open segment
        int close(bool will_sync = true);

        // sync open segment
        int sync(bool will_sync);

        // unlink segment
        int unlink();

        // truncate segment to last_index_kept
        int truncate(const int64_t last_index_kept);

        bool is_open() const {
            return _is_open;
        }

        int64_t bytes() const {
            return _bytes;
        }

        int64_t first_index() const {
            return _first_index;
        }

        int64_t last_index() const {
            return _last_index.load(mutil::memory_order_consume);
        }

        std::string file_name();

    private:
        friend class mutil::RefCountedThreadSafe<Segment>;

        ~Segment() {
            if (_fd >= 0) {
                ::close(_fd);
                _fd = -1;
            }
        }

        struct LogMeta {
            off_t offset;
            size_t length;
            int64_t term;
        };

        int _load_entry(off_t offset, EntryHeader *head, mutil::IOBuf *body,
                        size_t size_hint) const;

        int _get_meta(int64_t index, LogMeta *meta) const;

        int _truncate_meta_and_get_last(int64_t last);

        std::string _path;
        int64_t _bytes;
        int64_t _unsynced_bytes;
        mutable raft_mutex_t _mutex;
        int _fd;
        bool _is_open;
        const int64_t _first_index;
        mutil::atomic<int64_t> _last_index;
        int _checksum_type;
        std::vector<std::pair<int64_t/*offset*/, int64_t/*term*/> > _offset_and_term;
    };

// LogStorage use segmented append-only file, all data in disk, all index in memory.
// append one log entry, only cause one disk write, every disk write will call fsync().
//
// SegmentLog layout:
//      log_meta: record start_log
//      log_000001-0001000: closed segment
//      log_inprogress_0001001: open segment
    class SegmentLogStorage : public LogStorage {
    public:
        typedef std::map<int64_t, scoped_refptr<Segment> > SegmentMap;

        explicit SegmentLogStorage(const std::string &path, bool enable_sync = true)
                : _path(path), _first_log_index(1), _last_log_index(0), _checksum_type(0), _enable_sync(enable_sync) {}

        SegmentLogStorage()
                : _first_log_index(1), _last_log_index(0), _checksum_type(0), _enable_sync(true) {}

        virtual ~SegmentLogStorage() {}

        // init logstorage, check consistency and integrity
        virtual int init(ConfigurationManager *configuration_manager);

        // first log index in log
        virtual int64_t first_log_index() {
            return _first_log_index.load(mutil::memory_order_acquire);
        }

        // last log index in log
        virtual int64_t last_log_index();

        // get logentry by index
        virtual LogEntry *get_entry(const int64_t index);

        // get logentry's term by index
        virtual int64_t get_term(const int64_t index);

        // append entry to log
        int append_entry(const LogEntry *entry);

        // append entries to log and update IOMetric, return success append number
        virtual int append_entries(const std::vector<LogEntry *> &entries, IOMetric *metric);

        // delete logs from storage's head, [1, first_index_kept) will be discarded
        virtual int truncate_prefix(const int64_t first_index_kept);

        // delete uncommitted logs from storage's tail, (last_index_kept, infinity) will be discarded
        virtual int truncate_suffix(const int64_t last_index_kept);

        virtual int reset(const int64_t next_log_index);

        LogStorage *new_instance(const std::string &uri) const;

        mutil::Status gc_instance(const std::string &uri) const;

        SegmentMap segments() {
            MELON_SCOPED_LOCK(_mutex);
            return _segments;
        }

        void list_files(std::vector<std::string> *seg_files);

        void sync();

    private:
        scoped_refptr<Segment> open_segment();

        int save_meta(const int64_t log_index);

        int load_meta();

        int list_segments(bool is_empty);

        int load_segments(ConfigurationManager *configuration_manager);

        int get_segment(int64_t log_index, scoped_refptr<Segment> *ptr);

        void pop_segments(
                int64_t first_index_kept,
                std::vector<scoped_refptr<Segment> > *poped);

        void pop_segments_from_back(
                const int64_t last_index_kept,
                std::vector<scoped_refptr<Segment> > *popped,
                scoped_refptr<Segment> *last_segment);


        std::string _path;
        mutil::atomic<int64_t> _first_log_index;
        mutil::atomic<int64_t> _last_log_index;
        raft_mutex_t _mutex;
        SegmentMap _segments;
        scoped_refptr<Segment> _open_segment;
        int _checksum_type;
        bool _enable_sync;
    };

}  //  namespace melon::raft
