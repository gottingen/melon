//
// Created by liyinbin on 2021/4/17.
//

#include "abel/fiber/fiber_config.h"

namespace abel {

    fiber_config & fiber_config::set_worker_num(uint32_t n) {
        workers_per_group = n;
        return *this;
    }

    fiber_config & fiber_config::set_policy( std::shared_ptr<abel::core_affinity::affinity_policy> other) {
        policy = other;
        return *this;
    }

    fiber_config fiber_config::all_cores() {
        fiber_config cfg;
        cfg.workers_per_group = core_affinity::num_logical_cores()/2;
        cfg.groups = 2;
        cfg.policy = abel::core_affinity::affinity_policy::any_of(abel::core_affinity::all());
        return cfg;
    }
}  // namespace abel