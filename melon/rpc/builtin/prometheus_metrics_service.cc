// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.


#include <vector>
#include <iomanip>
#include <map>
#include "melon/rpc/controller.h"                // Controller
#include "melon/rpc/server.h"                    // Server
#include "melon/rpc/closure_guard.h"             // ClosureGuard
#include "melon/rpc/builtin/prometheus_metrics_service.h"
#include "melon/rpc/builtin/common.h"
#include "melon/metrics/all.h"
#include "melon/strings/str_format.h"
#include "melon/strings/utility.h"
#include "melon/strings/starts_with.h"
#include "melon/strings/ends_with.h"

namespace melon {
    DECLARE_int32(variable_latency_p1);
    DECLARE_int32(variable_latency_p2);
    DECLARE_int32(variable_latency_p3);
}

namespace melon::rpc {

    // Defined in server.cpp
    extern const char *const g_server_info_prefix;

    // This is a class that convert variable result to prometheus output.
    // Currently the output only includes gauge and summary for two
    // reasons:
    // 1) We cannot tell gauge and counter just from name and what's
    // more counter is just another gauge.
    // 2) Histogram and summary is equivalent except that histogram
    // calculates quantiles in the server side.
    class PrometheusMetricsDumper : public melon::variable_dumper {
    public:
        explicit PrometheusMetricsDumper(melon::cord_buf_builder *os,
                                         const std::string &server_prefix)
                : _os(os), _server_prefix(server_prefix) {
        }

        bool dump(const std::string &name, const std::string_view &desc) override;

    private:
        MELON_DISALLOW_COPY_AND_ASSIGN(PrometheusMetricsDumper);

        // Return true iff name ends with suffix output by LatencyRecorder.
        bool DumpLatencyRecorderSuffix(const std::string_view &name,
                                       const std::string_view &desc);

        // 6 is the number of variables in LatencyRecorder that indicating percentiles
        static const int NPERCENTILES = 6;

        struct SummaryItems {
            std::string latency_percentiles[NPERCENTILES];
            int64_t latency_avg;
            int64_t count;
            std::string metric_name;

            bool IsComplete() const { return !metric_name.empty(); }
        };

        const SummaryItems *ProcessLatencyRecorderSuffix(const std::string_view &name,
                                                         const std::string_view &desc);

    private:
        melon::cord_buf_builder *_os;
        const std::string _server_prefix;
        std::map<std::string, SummaryItems> _m;
    };

    bool PrometheusMetricsDumper::dump(const std::string &name,
                                       const std::string_view &desc) {
        if (!desc.empty() && desc[0] == '"') {
            // there is no necessary to monitor string in prometheus
            return true;
        }
        if (DumpLatencyRecorderSuffix(name, desc)) {
            // Has encountered name with suffix exposed by LatencyRecorder,
            // Leave it to DumpLatencyRecorderSuffix to output Summary.
            return true;
        }
        *_os << "# HELP " << name << '\n'
             << "# TYPE " << name << " gauge" << '\n'
             << name << " " << desc << '\n';
        return true;
    }

    const PrometheusMetricsDumper::SummaryItems *
    PrometheusMetricsDumper::ProcessLatencyRecorderSuffix(const std::string_view &name,
                                                          const std::string_view &desc) {
        static std::string latency_names[] = {
                melon::string_printf("_latency_%d", (int) melon::FLAGS_variable_latency_p1),
                melon::string_printf("_latency_%d", (int) melon::FLAGS_variable_latency_p2),
                melon::string_printf("_latency_%d", (int) melon::FLAGS_variable_latency_p3),
                "_latency_999", "_latency_9999", "_max_latency"
        };
        MELON_CHECK(NPERCENTILES == MELON_ARRAY_SIZE(latency_names));
        const std::string desc_str(desc.data(), desc.size());
        std::string_view metric_name(name);
        for (int i = 0; i < NPERCENTILES; ++i) {
            if (!melon::ends_with(metric_name, latency_names[i])) {
                continue;
            }
            metric_name.remove_suffix(latency_names[i].size());
            SummaryItems *si = &_m[melon::as_string(metric_name)];
            si->latency_percentiles[i] = desc_str;
            if (i == NPERCENTILES - 1) {
                // '_max_latency' is the last suffix name that appear in the sorted variable
                // list, which means all related percentiles have been gathered and we are
                // ready to output a Summary.
                si->metric_name = melon::as_string(metric_name);
            }
            return si;
        }
        // Get the average of latency in recent window size
        if (melon::ends_with(metric_name, "_latency")) {
            metric_name.remove_suffix(8);
            SummaryItems *si = &_m[melon::as_string(metric_name)];
            si->latency_avg = strtoll(desc_str.data(), nullptr, 10);
            return si;
        }
        if (melon::ends_with(metric_name, "_count")) {
            metric_name.remove_suffix(6);
            SummaryItems *si = &_m[melon::as_string(metric_name)];
            si->count = strtoll(desc_str.data(), nullptr, 10);
            return si;
        }
        return nullptr;
    }

    bool PrometheusMetricsDumper::DumpLatencyRecorderSuffix(
            const std::string_view &name,
            const std::string_view &desc) {
        if (!melon::starts_with(name, _server_prefix)) {
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
             << (double) (melon::FLAGS_variable_latency_p1) / 100 << "\"} "
             << si->latency_percentiles[0] << '\n'
             << si->metric_name << "{quantile=\""
             << (double) (melon::FLAGS_variable_latency_p2) / 100 << "\"} "
             << si->latency_percentiles[1] << '\n'
             << si->metric_name << "{quantile=\""
             << (double) (melon::FLAGS_variable_latency_p3) / 100 << "\"} "
             << si->latency_percentiles[2] << '\n'
             << si->metric_name << "{quantile=\"0.999\"} "
             << si->latency_percentiles[3] << '\n'
             << si->metric_name << "{quantile=\"0.9999\"} "
             << si->latency_percentiles[4] << '\n'
             << si->metric_name << "{quantile=\"1\"} "
             << si->latency_percentiles[5] << '\n'
             << si->metric_name << "_sum "
             // There is no sum of latency in variable output, just use
             // average * count as approximation
             << si->latency_avg * si->count << '\n'
             << si->metric_name << "_count " << si->count << '\n';
        return true;
    }

    void PrometheusMetricsService::default_method(::google::protobuf::RpcController *cntl_base,
                                                  const ::melon::rpc::MetricsRequest *,
                                                  ::melon::rpc::MetricsResponse *,
                                                  ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        cntl->http_response().set_content_type("text/plain");
        if (DumpPrometheusMetricsToCordBuf(&cntl->response_attachment()) != 0) {
            cntl->SetFailed("Fail to dump metrics");
            return;
        }
    }

    int DumpPrometheusMetricsToCordBuf(melon::cord_buf *output) {
        melon::cord_buf_builder os;
        melon::prometheus_dumper dumper(&os);
        const int ndump = melon::variable_base::dump_metrics(&dumper, nullptr);
        if (ndump < 0) {
            return -1;
        }
        os.move_to(*output);
        return 0;
    }


    /*
     * int DumpPrometheusMetricsToCordBuf(melon::cord_buf *output) {
        melon::cord_buf_builder os;
        PrometheusMetricsDumper dumper(&os, g_server_info_prefix);
        const int ndump = melon::variable_base::dump_exposed(&dumper, nullptr);
        if (ndump < 0) {
            return -1;
        }
        os.move_to(*output);
        return 0;
    }
     */

} // namespace melon::rpc
