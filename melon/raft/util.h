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

#include <zlib.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <stdlib.h>
#include <string>
#include <set>
#include <melon/utility/third_party/murmurhash3/murmurhash3.h>
#include <melon/base/endpoint.h>
#include <melon/base/scoped_lock.h>
#include <melon/base/fast_rand.h>
#include <melon/utility/time.h>
#include <turbo/log/logging.h>
#include <melon/base/iobuf.h>
#include <memory>
#include <melon/utility/memory/singleton.h>
#include <melon/utility/containers/doubly_buffered_data.h>
#include <turbo/crypto/crc32c.h>
#include <melon/utility/file_util.h>
#include <melon/fiber/fiber.h>
#include <melon/fiber/unstable.h>
#include <melon/fiber/countdown_event.h>
#include <melon/var/var.h>
#include <melon/raft/macros.h>
#include <melon/raft/raft.h>

namespace melon::var {
    namespace detail {

        class Percentile;

        typedef Window<IntRecorder, SERIES_IN_SECOND> RecorderWindow;
        typedef Window<Maxer<uint64_t>, SERIES_IN_SECOND> MaxUint64Window;
        typedef Window<Percentile, SERIES_IN_SECOND> PercentileWindow;

// For mimic constructor inheritance.
        class CounterRecorderBase {
        public:
            explicit CounterRecorderBase(time_t window_size);

            time_t window_size() const { return _avg_counter_window.window_size(); }

        protected:
            IntRecorder _avg_counter;
            Maxer<uint64_t> _max_counter;
            Percentile _counter_percentile;
            RecorderWindow _avg_counter_window;
            MaxUint64Window _max_counter_window;
            PercentileWindow _counter_percentile_window;

            PassiveStatus<int64_t> _total_times;
            PassiveStatus<int64_t> _qps;
            PassiveStatus<int64_t> _counter_p1;
            PassiveStatus<int64_t> _counter_p2;
            PassiveStatus<int64_t> _counter_p3;
            PassiveStatus<int64_t> _counter_999;  // 99.9%
            PassiveStatus<int64_t> _counter_9999; // 99.99%
            CDF _counter_cdf;
            PassiveStatus<Vector<int64_t, 4> > _counter_percentiles;
        };
    } // namespace detail

// Specialized structure to record counter.
// It's not a Variable, but it contains multiple var inside.
    class CounterRecorder : public detail::CounterRecorderBase {
        typedef detail::CounterRecorderBase Base;
    public:
        CounterRecorder() : Base(-1) {}

        explicit CounterRecorder(time_t window_size) : Base(window_size) {}

        explicit CounterRecorder(const mutil::StringPiece &prefix) : Base(-1) {
            expose(prefix);
        }

        CounterRecorder(const mutil::StringPiece &prefix,
                        time_t window_size) : Base(window_size) {
            expose(prefix);
        }

        CounterRecorder(const mutil::StringPiece &prefix1,
                        const mutil::StringPiece &prefix2) : Base(-1) {
            expose(prefix1, prefix2);
        }

        CounterRecorder(const mutil::StringPiece &prefix1,
                        const mutil::StringPiece &prefix2,
                        time_t window_size) : Base(window_size) {
            expose(prefix1, prefix2);
        }

        ~CounterRecorder() { hide(); }

        // Record the counter num.
        CounterRecorder &operator<<(int64_t count_num);

        // Expose all internal variables using `prefix' as prefix.
        // Returns 0 on success, -1 otherwise.
        // Example:
        //   CounterRecorder rec;
        //   rec.expose("foo_bar_add");     // foo_bar_add_avg_counter
        //                                    // foo_bar_add_max_counter
        //                                    // foo_bar_add_total_times
        //                                    // foo_bar_add_qps
        //   rec.expose("foo_bar", "apply");   // foo_bar_apply_avg_counter
        //                                    // foo_bar_apply_max_counter
        //                                    // foo_bar_apply_total_times
        //                                    // foo_bar_apply_qps
        int expose(const mutil::StringPiece &prefix) {
            return expose(mutil::StringPiece(), prefix);
        }

        int expose(const mutil::StringPiece &prefix1,
                   const mutil::StringPiece &prefix2);

        // Hide all internal variables, called in dtor as well.
        void hide();

        // Get the average counter num in recent |window_size| seconds
        // If |window_size| is absent, use the window_size to ctor.
        int64_t avg_counter(time_t window_size) const {
            return _avg_counter_window.get_value(window_size).get_average_int();
        }

        int64_t avg_counter() const { return _avg_counter_window.get_value().get_average_int(); }

        // Get p1/p2/p3/99.9-ile counter num in recent window_size-to-ctor seconds.
        Vector<int64_t, 4> counter_percentiles() const;

        // Get the max counter numer in recent window_size-to-ctor seconds.
        int64_t max_counter() const { return _max_counter_window.get_value(); }

        // Get the total number of recorded counter nums
        int64_t total_times() const { return _avg_counter.get_value().num; }

        // Get qps in recent |window_size| seconds. The `q' means counter nums.
        // recorded by operator<<().
        // If |window_size| is absent, use the window_size to ctor.
        int64_t qps(time_t window_size) const;

        int64_t qps() const { return _qps.get_value(); }

        // Get |ratio|-ile counter num in recent |window_size| seconds
        // E.g. 0.99 means 99%-ile
        int64_t counter_percentile(double ratio) const;

        // Get name of a sub-var.
        const std::string &avg_counter_name() const { return _avg_counter_window.name(); }

        const std::string &counter_percentiles_name() const { return _counter_percentiles.name(); }

        const std::string &counter_cdf_name() const { return _counter_cdf.name(); }

        const std::string &max_counter_name() const { return _max_counter_window.name(); }

        const std::string &total_times_name() const { return _total_times.name(); }

        const std::string &qps_name() const { return _qps.name(); }
    };

    std::ostream &operator<<(std::ostream &os, const CounterRecorder &);

}  // namespace melon::var

namespace melon::raft {
    class Closure;

// http://stackoverflow.com/questions/1493936/faster-approach-to-checking-for-an-all-zero-buffer-in-c
    inline bool is_zero(const char *buff, const size_t size) {
        if (size >= sizeof(uint64_t)) {
            return (0 == *(uint64_t *) buff) &&
                   (0 == memcmp(buff, buff + sizeof(uint64_t), size - sizeof(uint64_t)));
        } else if (size > 0) {
            return (0 == *(uint8_t *) buff) &&
                   (0 == memcmp(buff, buff + sizeof(uint8_t), size - sizeof(uint8_t)));
        } else {
            return 0;
        }
    }

    inline uint32_t murmurhash32(const void *key, int len) {
        uint32_t hash = 0;
        mutil::MurmurHash3_x86_32(key, len, 0, &hash);
        return hash;
    }

    inline uint32_t murmurhash32(const mutil::IOBuf &buf) {
        mutil::MurmurHash3_x86_32_Context ctx;
        mutil::MurmurHash3_x86_32_Init(&ctx, 0);
        const size_t block_num = buf.backing_block_num();
        for (size_t i = 0; i < block_num; ++i) {
            mutil::StringPiece sp = buf.backing_block(i);
            if (!sp.empty()) {
                mutil::MurmurHash3_x86_32_Update(&ctx, sp.data(), sp.size());
            }
        }
        uint32_t hash = 0;
        mutil::MurmurHash3_x86_32_Final(&hash, &ctx);
        return hash;
    }

    inline uint32_t crc32(const void *key, int len) {
        return (uint32_t)turbo::compute_crc32c(std::string_view{(const char *) key, len});
    }

    inline uint32_t crc32(const mutil::IOBuf &buf) {
        turbo::CRC32C hash;
        const size_t block_num = buf.backing_block_num();
        for (size_t i = 0; i < block_num; ++i) {
            mutil::StringPiece sp = buf.backing_block(i);
            if (!sp.empty()) {
                hash = turbo::extend_crc32c(hash, std::string_view{sp.data(), sp.size()});
            }
        }
        return (uint32_t)hash;
    }

    // Start a fiber to run closure
    void run_closure_in_fiber(::google::protobuf::Closure *closure,
                                bool in_pthread = false);

    struct RunClosureInFiber {
        void operator()(google::protobuf::Closure *done) {
            return run_closure_in_fiber(done);
        }
    };

    typedef std::unique_ptr<google::protobuf::Closure, RunClosureInFiber>
            AsyncClosureGuard;

    // Start a fiber to run closure without signal other worker thread to steal
    // it. You should call fiber_flush() at last.
    void run_closure_in_fiber_nosig(::google::protobuf::Closure *closure,
                                      bool in_pthread = false);

    struct RunClosureInFiberNoSig {
        void operator()(google::protobuf::Closure *done) {
            return run_closure_in_fiber_nosig(done);
        }
    };

    ssize_t file_pread(mutil::IOPortal *portal, int fd, off_t offset, size_t size);

    ssize_t file_pwrite(const mutil::IOBuf &data, int fd, off_t offset);

    // unsequence file data, reduce the overhead of copy some files have hole.
    class FileSegData {
    public:
        // for reader
        FileSegData(const mutil::IOBuf &data)
                : _data(data), _seg_header(mutil::IOBuf::INVALID_AREA), _seg_offset(0), _seg_len(0) {}

        // for writer
        FileSegData() : _seg_header(mutil::IOBuf::INVALID_AREA), _seg_offset(0), _seg_len(0) {}

        ~FileSegData() {
            close();
        }

        // writer append
        void append(const mutil::IOBuf &data, uint64_t offset);

        void append(void *data, uint64_t offset, uint32_t len);

        // writer get
        mutil::IOBuf &data() {
            close();
            return _data;
        }

        // read next, NEED clear data when call next in loop
        size_t next(uint64_t *offset, mutil::IOBuf *data);

    private:
        void close();

        mutil::IOBuf _data;
        mutil::IOBuf::Area _seg_header;
        uint64_t _seg_offset;
        uint32_t _seg_len;
    };

// A special Closure which provides synchronization primitives
    class SynchronizedClosure : public Closure {
    public:
        SynchronizedClosure() : _event(1) {}

        SynchronizedClosure(int num_signal) : _event(num_signal) {}

        // Implements melon::raft::Closure
        void Run() {
            _event.signal();
        }

        // Block the thread until Run() has been called
        void wait() { _event.wait(); }

        // Reset the event
        void reset() {
            status().reset();
            _event.reset();
        }

    private:
        fiber::CountdownEvent _event;
    };

}  //  namespace melon::raft
