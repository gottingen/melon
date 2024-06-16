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



#include <limits>
#include <melon/base/macros.h>
#include <melon/rpc/controller.h>
#include <melon/rpc/details/server_private_accessor.h>
#include <melon/rpc/details/method_status.h>

namespace melon {

static int cast_int(void* arg) {
    return *(int*)arg;
}

static int cast_cl(void* arg) {
    auto cl = static_cast<std::unique_ptr<ConcurrencyLimiter>*>(arg)->get();
    if (cl) {
        return cl->MaxConcurrency();
    }
    return 0;
}

MethodStatus::MethodStatus()
    : _nconcurrency(0)
    , _nconcurrency_var(cast_int, &_nconcurrency)
    , _eps_var(&_nerror_var)
    , _max_concurrency_var(cast_cl, &_cl)
{
}

MethodStatus::~MethodStatus() {
}

int MethodStatus::Expose(const std::string_view& prefix) {
    if (_nconcurrency_var.expose_as(prefix, "concurrency") != 0) {
        return -1;
    }
    if (_nerror_var.expose_as(prefix, "error") != 0) {
        return -1;
    }
    if (_eps_var.expose_as(prefix, "eps") != 0) {
        return -1;
    }
    if (_latency_rec.expose(prefix) != 0) {
        return -1;
    }
    if (_cl) {
        if (_max_concurrency_var.expose_as(prefix, "max_concurrency") != 0) {
            return -1;
        }
    }
    return 0;
}

template <typename T>
void OutputTextValue(std::ostream& os,
                     const char* prefix,
                     const T& value) {
    os << prefix << value << "\n";
}

template <typename T>
void OutputValue(std::ostream& os,
                 const char* prefix,
                 const std::string& var_name,
                 const T& value,
                 const DescribeOptions& options,
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
    std::ostream &os, const DescribeOptions& options) const {
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

void MethodStatus::SetConcurrencyLimiter(ConcurrencyLimiter* cl) {
    _cl.reset(cl);
}

ConcurrencyRemover::~ConcurrencyRemover() {
    if (_status) {
        _status->OnResponded(_c->ErrorCode(), mutil::cpuwide_time_us() - _received_us);
        _status = NULL;
    }
    ServerPrivateAccessor(_c->server()).RemoveConcurrency(_c);
}

}  // namespace melon
