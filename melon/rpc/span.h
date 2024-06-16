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



// NOTE: RPC users are not supposed to include this file.

#ifndef MELON_RPC_SPAN_H_
#define MELON_RPC_SPAN_H_

#include <stdint.h>
#include <string>
#include <deque>
#include <ostream>
#include <melon/base/macros.h>
#include <melon/base/endpoint.h>
#include <melon/utility/string_splitter.h>
#include <melon/var/collector.h>
#include <melon/fiber/task_meta.h>
#include <melon/proto/rpc/options.pb.h>                 // ProtocolType
#include <melon/proto/rpc/span.pb.h>
#include <turbo/strings/str_format.h>

namespace fiber {
    extern __thread fiber::LocalStorage tls_bls;
}


namespace melon {

    DECLARE_bool(enable_rpcz);

// Collect information required by /rpcz and tracing system whose idea is
// described in http://static.googleusercontent.com/media/research.google.com/en//pubs/archive/36356.pdf
    class Span : public melon::var::Collected {
        friend class SpanDB;

        struct Forbidden {
        };
    public:
        // Call CreateServerSpan/CreateClientSpan instead.
        Span(Forbidden) {}

        ~Span() {}

        // Create a span to track a request inside server.
        static Span *CreateServerSpan(
                const std::string &full_method_name,
                uint64_t trace_id, uint64_t span_id, uint64_t parent_span_id,
                int64_t base_real_us);

        // Create a span without name to track a request inside server.
        static Span *CreateServerSpan(
                uint64_t trace_id, uint64_t span_id, uint64_t parent_span_id,
                int64_t base_real_us);

        // Clear all annotations and reset name of the span.
        void ResetServerSpanName(const std::string &name);

        // Create a span to track a request inside channel.
        static Span *CreateClientSpan(const std::string &full_method_name,
                                      int64_t base_real_us);

        static void Submit(Span *span, int64_t cpuwide_time_us);

        // Set tls parent.
        void AsParent() {
            fiber::tls_bls.rpcz_parent_span = this;
        }

        // Add log with time.
        void Annotate(const std::string &info);

        // When length <= 0, use strlen instead.
        template<typename ...Args>
        void Annotate(const turbo::FormatSpec<Args...> &fmt, Args... args) {
            Annotate(turbo::str_format(fmt, args...));
        }

        void AnnotateCStr(const char *cstr, size_t length);

        // #child spans, Not O(1)
        size_t CountClientSpans() const;

        int64_t GetStartRealTimeUs() const;

        int64_t GetEndRealTimeUs() const;

        void set_log_id(uint64_t cid) { _log_id = cid; }

        void set_base_cid(fiber_session_t id) { _base_cid = id; }

        void set_ending_cid(fiber_session_t id) { _ending_cid = id; }

        void set_remote_side(const mutil::EndPoint &pt) { _remote_side = pt; }

        void set_protocol(ProtocolType p) { _protocol = p; }

        void set_error_code(int error_code) { _error_code = error_code; }

        void set_request_size(int size) { _request_size = size; }

        void set_response_size(int size) { _response_size = size; }

        void set_async(bool async) { _async = async; }

        void set_base_real_us(int64_t tm) { _base_real_us = tm; }

        void set_received_us(int64_t tm) { _received_real_us = tm + _base_real_us; }

        void set_start_parse_us(int64_t tm) { _start_parse_real_us = tm + _base_real_us; }

        void set_start_callback_us(int64_t tm) { _start_callback_real_us = tm + _base_real_us; }

        void set_start_send_us(int64_t tm) { _start_send_real_us = tm + _base_real_us; }

        void set_sent_us(int64_t tm) { _sent_real_us = tm + _base_real_us; }

        Span *local_parent() const { return _local_parent; }

        static Span *tls_parent() {
            return (Span *) fiber::tls_bls.rpcz_parent_span;
        }

        uint64_t trace_id() const { return _trace_id; }

        uint64_t parent_span_id() const { return _parent_span_id; }

        uint64_t span_id() const { return _span_id; }

        uint64_t log_id() const { return _log_id; }

        fiber_session_t base_cid() const { return _base_cid; }

        fiber_session_t ending_cid() const { return _ending_cid; }

        const mutil::EndPoint &remote_side() const { return _remote_side; }

        SpanType type() const { return _type; }

        ProtocolType protocol() const { return _protocol; }

        int error_code() const { return _error_code; }

        int request_size() const { return _request_size; }

        int response_size() const { return _response_size; }

        int64_t received_real_us() const { return _received_real_us; }

        int64_t start_parse_real_us() const { return _start_parse_real_us; }

        int64_t start_callback_real_us() const { return _start_callback_real_us; }

        int64_t start_send_real_us() const { return _start_send_real_us; }

        int64_t sent_real_us() const { return _sent_real_us; }

        bool async() const { return _async; }

        const std::string &full_method_name() const { return _full_method_name; }

        const std::string &info() const { return _info; }

    private:
        DISALLOW_COPY_AND_ASSIGN(Span);

        void dump_and_destroy(size_t round_index);

        void destroy();

        melon::var::CollectorSpeedLimit *speed_limit();

        melon::var::CollectorPreprocessor *preprocessor();

        void EndAsParent() {
            if (this == (Span *) fiber::tls_bls.rpcz_parent_span) {
                fiber::tls_bls.rpcz_parent_span = NULL;
            }
        }

        uint64_t _trace_id;
        uint64_t _span_id;
        uint64_t _parent_span_id;
        uint64_t _log_id;
        fiber_session_t _base_cid;
        fiber_session_t _ending_cid;
        mutil::EndPoint _remote_side;
        SpanType _type;
        bool _async;
        ProtocolType _protocol;
        int _error_code;
        int _request_size;
        int _response_size;
        int64_t _base_real_us;
        int64_t _received_real_us;
        int64_t _start_parse_real_us;
        int64_t _start_callback_real_us;
        int64_t _start_send_real_us;
        int64_t _sent_real_us;
        std::string _full_method_name;
        // Format:
        //   time1_us \s annotation1 <SEP>
        //   time2_us \s annotation2 <SEP>
        //   ...
        std::string _info;

        Span *_local_parent;
        Span *_next_client;
        Span *_tls_next;
    };

    // Extract name and annotations from Span::info()
    class SpanInfoExtractor {
    public:
        SpanInfoExtractor(const char *info);

        bool PopAnnotation(int64_t before_this_time,
                           int64_t *time, std::string *annotation);

    private:
        mutil::StringSplitter _sp;
    };

    // These two functions can be used for composing TRACEPRINT as well as hiding
    // span implementations.
    bool CanAnnotateSpan();

    void AnnotateSpan(const std::string &info);

    template<typename ...Args>
    inline void AnnotateSpan(const turbo::FormatSpec<Args...> &fmt, Args... args) {
        AnnotateSpan(turbo::str_format(fmt, args...));
    }


    class SpanFilter {
    public:
        virtual bool Keep(const BriefSpan &) = 0;
    };

    class SpanDB;

    // Find a span by its trace_id and span_id, serialize it into `span'.
    int FindSpan(uint64_t trace_id, uint64_t span_id, RpczSpan *span);

    // Find spans by their trace_id, serialize them into `out'
    void FindSpans(uint64_t trace_id, std::deque<RpczSpan> *out);

    // Put at most `max_scan' spans before `before_this_time' into `out'.
    // If filter is not NULL, only push spans that make SpanFilter::Keep()
    // true.
    void ListSpans(int64_t before_this_time, size_t max_scan,
                   std::deque<BriefSpan> *out, SpanFilter *filter);

    void DescribeSpanDB(std::ostream &os);

    SpanDB *LoadSpanDBFromFile(const char *filepath);

    int FindSpan(SpanDB *db, uint64_t trace_id, uint64_t span_id, RpczSpan *span);

    void FindSpans(SpanDB *db, uint64_t trace_id, std::deque<RpczSpan> *out);

    void ListSpans(SpanDB *db, int64_t before_this_time, size_t max_scan,
                   std::deque<BriefSpan> *out, SpanFilter *filter);

    // Check this function first before creating a span.
    // If rpcz of upstream is enabled, local rpcz is enabled automatically.
    inline bool IsTraceable(bool is_upstream_traced) {
        extern melon::var::CollectorSpeedLimit g_span_sl;
        return is_upstream_traced ||
               (FLAGS_enable_rpcz && melon::var::is_collectable(&g_span_sl));
    }

} // namespace melon


#endif // MELON_RPC_SPAN_H_
