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



#include <vector>
#include <iomanip>
#include <map>
#include "melon/rpc/controller.h"                // Controller
#include "melon/rpc/server.h"                    // Server
#include "melon/rpc/closure_guard.h"             // ClosureGuard
#include "melon/builtin/prometheus_metrics_service.h"
#include "melon/builtin/common.h"
#include "melon/var/var.h"

namespace melon::var {
    DECLARE_int32(bvar_latency_p1);
    DECLARE_int32(bvar_latency_p2);
    DECLARE_int32(bvar_latency_p3);
    DECLARE_int32(bvar_max_dump_multi_dimension_metric_number);
}

namespace melon {

    // Defined in server.cpp
    extern const char *const g_server_info_prefix;

    // This is a class that convert var result to prometheus output.
    // Currently the output only includes gauge and summary for two
    // reasons:
    // 1) We cannot tell gauge and counter just from name and what's
    // more counter is just another gauge.
    // 2) Histogram and summary is equivalent except that histogram
    // calculates quantiles in the server side.
    class PrometheusMetricsDumper : public melon::var::Dumper {
    public:
        explicit PrometheusMetricsDumper(mutil::IOBufBuilder *os,
                                         const std::string &server_prefix)
                : _os(os), _server_prefix(server_prefix) {
        }

        bool dump(const std::string &name, const mutil::StringPiece &desc) override;

    private:
        DISALLOW_COPY_AND_ASSIGN(PrometheusMetricsDumper);

        // Return true iff name ends with suffix output by LatencyRecorder.
        bool DumpLatencyRecorderSuffix(const mutil::StringPiece &name,
                                       const mutil::StringPiece &desc);

        // 6 is the number of bvars in LatencyRecorder that indicating percentiles
        static const int NPERCENTILES = 6;

        struct SummaryItems {
            std::string latency_percentiles[NPERCENTILES];
            int64_t latency_avg;
            int64_t count;
            std::string metric_name;

            bool IsComplete() const { return !metric_name.empty(); }
        };

        const SummaryItems *ProcessLatencyRecorderSuffix(const mutil::StringPiece &name,
                                                         const mutil::StringPiece &desc);

    private:
        mutil::IOBufBuilder *_os;
        const std::string _server_prefix;
        std::map<std::string, SummaryItems> _m;
    };

    mutil::StringPiece GetMetricsName(const std::string &name) {
        auto pos = name.find_first_of('{');
        int size = (pos == std::string::npos) ? name.size() : pos;
        return mutil::StringPiece(name.data(), size);
    }

    bool PrometheusMetricsDumper::dump(const std::string &name,
                                       const mutil::StringPiece &desc) {
        if (!desc.empty() && desc[0] == '"') {
            // there is no necessary to monitor string in prometheus
            return true;
        }
        if (DumpLatencyRecorderSuffix(name, desc)) {
            // Has encountered name with suffix exposed by LatencyRecorder,
            // Leave it to DumpLatencyRecorderSuffix to output Summary.
            return true;
        }

        auto metrics_name = GetMetricsName(name);

        *_os << "# HELP " << metrics_name << '\n'
             << "# TYPE " << metrics_name << " gauge" << '\n'
             << name << " " << desc << '\n';
        return true;
    }

    const PrometheusMetricsDumper::SummaryItems *
    PrometheusMetricsDumper::ProcessLatencyRecorderSuffix(const mutil::StringPiece &name,
                                                          const mutil::StringPiece &desc) {
        static std::string latency_names[] = {
                mutil::string_printf("_latency_%d", (int) melon::var::FLAGS_bvar_latency_p1),
                mutil::string_printf("_latency_%d", (int) melon::var::FLAGS_bvar_latency_p2),
                mutil::string_printf("_latency_%d", (int) melon::var::FLAGS_bvar_latency_p3),
                "_latency_999", "_latency_9999", "_max_latency"
        };
        MCHECK(NPERCENTILES == arraysize(latency_names));
        const std::string desc_str = desc.as_string();
        mutil::StringPiece metric_name(name);
        for (int i = 0; i < NPERCENTILES; ++i) {
            if (!metric_name.ends_with(latency_names[i])) {
                continue;
            }
            metric_name.remove_suffix(latency_names[i].size());
            SummaryItems *si = &_m[metric_name.as_string()];
            si->latency_percentiles[i] = desc_str;
            if (i == NPERCENTILES - 1) {
                // '_max_latency' is the last suffix name that appear in the sorted var
                // list, which means all related percentiles have been gathered and we are
                // ready to output a Summary.
                si->metric_name = metric_name.as_string();
            }
            return si;
        }
        // Get the average of latency in recent window size
        if (metric_name.ends_with("_latency")) {
            metric_name.remove_suffix(8);
            SummaryItems *si = &_m[metric_name.as_string()];
            si->latency_avg = strtoll(desc_str.data(), NULL, 10);
            return si;
        }
        if (metric_name.ends_with("_count")) {
            metric_name.remove_suffix(6);
            SummaryItems *si = &_m[metric_name.as_string()];
            si->count = strtoll(desc_str.data(), NULL, 10);
            return si;
        }
        return NULL;
    }

    bool PrometheusMetricsDumper::DumpLatencyRecorderSuffix(
            const mutil::StringPiece &name,
            const mutil::StringPiece &desc) {
        if (!name.starts_with(_server_prefix)) {
            return false;
        }
        const SummaryItems *si = ProcessLatencyRecorderSuffix(name, desc);
        if (!si) {
            return false;
        }
        if (!si->IsComplete()) {
            return true;
        }
        *_os << "# HELP " << si->metric_name << '\n'
             << "# TYPE " << si->metric_name << " summary\n"
             << si->metric_name << "{quantile=\""
             << (double) (melon::var::FLAGS_bvar_latency_p1) / 100 << "\"} "
             << si->latency_percentiles[0] << '\n'
             << si->metric_name << "{quantile=\""
             << (double) (melon::var::FLAGS_bvar_latency_p2) / 100 << "\"} "
             << si->latency_percentiles[1] << '\n'
             << si->metric_name << "{quantile=\""
             << (double) (melon::var::FLAGS_bvar_latency_p3) / 100 << "\"} "
             << si->latency_percentiles[2] << '\n'
             << si->metric_name << "{quantile=\"0.999\"} "
             << si->latency_percentiles[3] << '\n'
             << si->metric_name << "{quantile=\"0.9999\"} "
             << si->latency_percentiles[4] << '\n'
             << si->metric_name << "{quantile=\"1\"} "
             << si->latency_percentiles[5] << '\n'
             << si->metric_name << "{quantile=\"avg\"} "
             << si->latency_avg << '\n'
             << si->metric_name << "_sum "
             // There is no sum of latency in var output, just use
             // average * count as approximation
             << si->latency_avg * si->count << '\n'
             << si->metric_name << "_count " << si->count << '\n';
        return true;
    }

    void PrometheusMetricsService::default_method(::google::protobuf::RpcController *cntl_base,
                                                  const ::melon::MetricsRequest *,
                                                  ::melon::MetricsResponse *,
                                                  ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        cntl->http_response().set_content_type("text/plain");
        if (DumpPrometheusMetricsToIOBuf(&cntl->response_attachment()) != 0) {
            cntl->SetFailed("Fail to dump metrics");
            return;
        }
    }

    int DumpPrometheusMetricsToIOBuf(mutil::IOBuf *output) {
        mutil::IOBufBuilder os;
        PrometheusMetricsDumper dumper(&os, g_server_info_prefix);
        const int ndump = melon::var::Variable::dump_exposed(&dumper, NULL);
        if (ndump < 0) {
            return -1;
        }
        os.move_to(*output);

        if (melon::var::FLAGS_bvar_max_dump_multi_dimension_metric_number > 0) {
            PrometheusMetricsDumper dumper_md(&os, g_server_info_prefix);
            const int ndump_md = melon::var::MVariable::dump_exposed(&dumper_md, NULL);
            if (ndump_md < 0) {
                return -1;
            }
            output->append(mutil::IOBuf::Movable(os.buf()));
        }
        return 0;
    }

} // namespace melon
