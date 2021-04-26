//
// Created by liyinbin on 2021/4/17.
//

#ifndef ABEL_FIBER_FIBER_CONFIG_H_
#define ABEL_FIBER_FIBER_CONFIG_H_

#include <memory>
#include <string>

#include "abel/functional/function.h"
#include "abel/thread/thread.h"

namespace abel {
    struct fiber_config {

        uint32_t scheduling_group_size{16};

        uint32_t groups{2};

        bool numa_aware{false};

        int32_t work_stealing_ratio{16};

        int32_t concurrency_hint{0};

        int32_t fiber_stack_size{131072};

        bool fiber_stack_enable_guard_page{true};

        std::string fiber_worker_accessible_cpus{""};

        std::string fiber_worker_inaccessible_cpus{""};

        bool fiber_worker_disallow_cpu_migration{false};

        bool ignore_inaccessible_cpus{true};

        int32_t cross_numa_work_stealing_ratio{0};

        std::string fiber_scheduling_optimize_for{""};

        int32_t fiber_run_queue_size{65536};

        static fiber_config& get_global_fiber_config();

        std::shared_ptr<abel::core_affinity::affinity_policy> policy;

        fiber_config &set_worker_num(uint32_t n);

        fiber_config &set_policy(std::shared_ptr<abel::core_affinity::affinity_policy> policy);

        static fiber_config all_cores();

    };

}  // namespace abel

#endif  // ABEL_FIBER_FIBER_CONFIG_H_
