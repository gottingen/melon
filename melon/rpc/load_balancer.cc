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



#include <gflags/gflags.h>
#include "melon/rpc/reloadable_flags.h"
#include "melon/rpc/load_balancer.h"


namespace melon {

    DEFINE_bool(show_lb_in_vars, false, "Describe LoadBalancers in vars");
    DEFINE_int32(default_weight_of_wlb, 0, "Default weight value of Weighted LoadBalancer(wlb). "
                                           "wlb policy degradation is enabled when default_weight_of_wlb > 0 to avoid some "
                                           "problems when user is using wlb but forgot to set the weights of some of their "
                                           "downstream instances. Then these instances will be set default_weight_of_wlb as "
                                           "their weights. wlb policy degradation is not enabled by default.");
    MELON_VALIDATE_GFLAG(show_lb_in_vars, PassValidate);

    // For assigning unique names for lb.
    static mutil::static_atomic<int> g_lb_counter = MUTIL_STATIC_ATOMIC_INIT(0);

    void SharedLoadBalancer::DescribeLB(std::ostream &os, void *arg) {
        (static_cast<SharedLoadBalancer *>(arg))->Describe(os, DescribeOptions());
    }

    void SharedLoadBalancer::ExposeLB() {
        bool changed = false;
        _st_mutex.lock();
        if (!_exposed) {
            _exposed = true;
            changed = true;
        }
        _st_mutex.unlock();
        if (changed) {
            char name[32];
            snprintf(name, sizeof(name), "_load_balancer_%d", g_lb_counter.fetch_add(
                    1, mutil::memory_order_relaxed));
            _st.expose(name);
        }
    }

    SharedLoadBalancer::SharedLoadBalancer()
            : _lb(NULL), _weight_sum(0), _exposed(false), _st(DescribeLB, this) {
    }

    SharedLoadBalancer::~SharedLoadBalancer() {
        _st.hide();
        if (_lb) {
            _lb->Destroy();
            _lb = NULL;
        }
    }

    int SharedLoadBalancer::Init(const char *lb_protocol) {
        std::string lb_name;
        mutil::StringPiece lb_params;
        if (!ParseParameters(lb_protocol, &lb_name, &lb_params)) {
            MLOG(FATAL) << "Fail to parse this load balancer protocol '" << lb_protocol << '\'';
            return -1;
        }
        const LoadBalancer *lb = LoadBalancerExtension()->Find(lb_name.c_str());
        if (lb == NULL) {
            MLOG(FATAL) << "Fail to find LoadBalancer by `" << lb_name << "'";
            return -1;
        }
        _lb = lb->New(lb_params);
        if (_lb == NULL) {
            MLOG(FATAL) << "Fail to new LoadBalancer";
            return -1;
        }
        if (FLAGS_show_lb_in_vars && !_exposed) {
            ExposeLB();
        }
        return 0;
    }

    void SharedLoadBalancer::Describe(std::ostream &os,
                                      const DescribeOptions &options) {
        if (_lb == NULL) {
            os << "lb=NULL";
        } else {
            _lb->Describe(os, options);
        }
    }

    bool SharedLoadBalancer::ParseParameters(const mutil::StringPiece &lb_protocol,
                                             std::string *lb_name,
                                             mutil::StringPiece *lb_params) {
        lb_name->clear();
        lb_params->clear();
        if (lb_protocol.empty()) {
            return false;
        }
        const char separator = ':';
        size_t pos = lb_protocol.find(separator);
        if (pos == std::string::npos) {
            lb_name->append(lb_protocol.data(), lb_protocol.size());
        } else {
            lb_name->append(lb_protocol.data(), pos);
            if (pos < lb_protocol.size() - sizeof(separator)) {
                *lb_params = lb_protocol.substr(pos + sizeof(separator));
            }
        }

        return true;
    }

} // namespace melon
