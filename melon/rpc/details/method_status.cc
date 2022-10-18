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


#include <limits>
#include "melon/base/profile.h"
#include "melon/rpc/controller.h"
#include "melon/rpc/details/server_private_accessor.h"
#include "melon/rpc/details/method_status.h"

namespace melon::rpc {

    static int cast_int(void *arg) {
        return *(int *) arg;
    }

    static int cast_cl(void *arg) {
        auto cl = static_cast<std::unique_ptr<ConcurrencyLimiter> *>(arg)->get();
        if (cl) {
            return cl->MaxConcurrency();
        }
        return 0;
    }

    MethodStatus::MethodStatus()
            : _nconcurrency(0), _nconcurrency_var(cast_int, &_nconcurrency), _eps_var(&_nerror_var),
              _max_concurrency_var(cast_cl, &_cl) {
    }

    MethodStatus::~MethodStatus() {
    }

    int MethodStatus::Expose(const std::string_view &prefix) {

        std::vector<double> buckets{10, 30, 50, 80, 100,  150, 200, 300, 400, 500, 700, 800, 1000, 1500, 2000};
        if (_his_latency.expose_as(std::string(prefix.data(), prefix.size()), "histogram_latency", "", buckets) != 0) {
            return -1;
        }

        if (_qps.expose_as(std::string(prefix.data(), prefix.size()), "counter_qps", "", {}, static_cast<display_filter>(DISPLAY_ON_METRICS | DISPLAY_ON_ALL)) != 0) {
            return -1;
        }

        if (_nconcurrency_var.expose_as(prefix, "concurrency", "", {}, static_cast<display_filter>(DISPLAY_ON_METRICS | DISPLAY_ON_ALL)) != 0) {
            return -1;
        }
        if (_nerror_var.expose_as(prefix, "error", "",{}, static_cast<display_filter>(DISPLAY_ON_METRICS | DISPLAY_ON_ALL)) != 0) {
            return -1;
        }
        if (_eps_var.expose_as(prefix, "eps", "") != 0) {
            return -1;
        }
        if (_latency_rec.expose(prefix) != 0) {
            return -1;
        }
        if (_cl) {
            if (_max_concurrency_var.expose_as(prefix, "max_concurrency", "", {}, static_cast<display_filter>(DISPLAY_ON_METRICS | DISPLAY_ON_ALL)) != 0) {
                return -1;
            }
        }
        return 0;
    }

    template<typename T>
    void OutputTextValue(std::ostream &os,
                         const char *prefix,
                         const T &value) {
        os << prefix << value << "\n";
    }

    template<typename T>
    void OutputValue(std::ostream &os,
                     const char *prefix,
                     const std::string &var_name,
                     const T &value,
                     const DescribeOptions &options,
                     bool expand) {
        if (options.use_html) {
            os << "<p class=\"variable";
            if (expand) {
                os << " default_expand";
            }
            os << "\">" << prefix << "<span id=\"value-" << var_name << "\">"
               << value
               << "</span></p><div class=\"detail\"><div id=\"" << var_name
               << "\" class=\"flot-placeholder\"></div></div>\n";
        } else {
            return OutputTextValue(os, prefix, value);
        }
    }

    void MethodStatus::Describe(
            std::ostream &os, const DescribeOptions &options) const {
        // success requests
        OutputValue(os, "count: ", _latency_rec.count_name(), _latency_rec.count(),
                    options, false);
        const int64_t qps = _latency_rec.qps();
        const bool expand = (qps != 0);
        OutputValue(os, "qps: ", _latency_rec.qps_name(), _latency_rec.qps(),
                    options, expand);

        // errorous requests
        OutputValue(os, "error: ", _nerror_var.name(), _nerror_var.get_value(),
                    options, false);
        OutputValue(os, "eps: ", _eps_var.name(),
                    _eps_var.get_value(1), options, false);

        // latencies
        OutputValue(os, "latency: ", _latency_rec.latency_name(),
                    _latency_rec.latency(), options, false);
        if (options.use_html) {
            OutputValue(os, "latency_percentiles: ",
                        _latency_rec.latency_percentiles_name(),
                        _latency_rec.latency_percentiles(), options, false);
            OutputValue(os, "latency_cdf: ", _latency_rec.latency_cdf_name(),
                        "click to view", options, expand);
        } else {
            OutputTextValue(os, "latency_50: ",
                            _latency_rec.latency_percentile(0.5));
            OutputTextValue(os, "latency_90: ",
                            _latency_rec.latency_percentile(0.9));
            OutputTextValue(os, "latency_99: ",
                            _latency_rec.latency_percentile(0.99));
            OutputTextValue(os, "latency_999: ",
                            _latency_rec.latency_percentile(0.999));
            OutputTextValue(os, "latency_9999: ",
                            _latency_rec.latency_percentile(0.9999));
        }
        OutputValue(os, "max_latency: ", _latency_rec.max_latency_name(),
                    _latency_rec.max_latency(), options, false);

        // Concurrency
        OutputValue(os, "concurrency: ", _nconcurrency_var.name(),
                    _nconcurrency, options, false);
        if (_cl) {
            OutputValue(os, "max_concurrency: ", _max_concurrency_var.name(),
                        MaxConcurrency(), options, false);
        }
    }

    void MethodStatus::SetConcurrencyLimiter(ConcurrencyLimiter *cl) {
        _cl.reset(cl);
    }

    ConcurrencyRemover::~ConcurrencyRemover() {
        if (_status) {
            _status->OnResponded(_c->ErrorCode(), melon::get_current_time_micros() - _received_us);
            _status = nullptr;
        }
        ServerPrivateAccessor(_c->server()).RemoveConcurrency(_c);
    }

}  // namespace melon::rpc
