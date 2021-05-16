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

    enum class schedule_type {
        eComputeHeavy,
        eCompute,
        eNormal,
        eIo,
        eIoHeavy
    };

    struct fiber_config {

        size_t workers_per_group{4};

        size_t scheduling_groups{2};

        bool enable_numa_aware{false};

        size_t work_stealing_ratio{16};

        size_t concurrency_hint{0};

        size_t fiber_stack_size{131072};

        bool fiber_stack_enable_guard_page{true};

        std::string fiber_worker_accessible_cpus{""};

        std::string fiber_worker_inaccessible_cpus{""};

        bool fiber_worker_disallow_cpu_migration{false};

        bool ignore_inaccessible_cpus{true};

        size_t cross_numa_work_stealing_ratio{0};

        std::string fiber_scheduling_optimize_for{""};

        size_t fiber_run_queue_size{65536};

        std::shared_ptr<abel::core_affinity::affinity_policy> policy;

        fiber_config &set_worker_num(uint32_t n);

        fiber_config &set_policy(std::shared_ptr<abel::core_affinity::affinity_policy> policy);

        static fiber_config all_cores();

        static fiber_config make_fiber_conf(schedule_type profile,
                                            std::size_t numa_domains,
                                            std::size_t available_processors,
                                            std::size_t desired_concurrency);

        static fiber_config &get_global_fiber_config();
    };

}  // namespace abel

#endif  // ABEL_FIBER_FIBER_CONFIG_H_
