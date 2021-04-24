//
// Created by liyinbin on 2021/4/17.
//

#ifndef ABEL_FIBER_FIBER_CONFIG_H_
#define ABEL_FIBER_FIBER_CONFIG_H_

#include <memory>
#include "abel/functional/function.h"
#include "abel/thread/thread.h"
namespace abel {


    struct fiber_config {

        uint32_t workers_per_group;

        uint32_t groups;

        std::shared_ptr<abel::core_affinity::affinity_policy> policy;

        fiber_config & set_worker_num(uint32_t n);

        fiber_config & set_policy( std::shared_ptr<abel::core_affinity::affinity_policy> policy);

        static fiber_config all_cores();

    };

}  // namespace abel

#endif  // ABEL_FIBER_FIBER_CONFIG_H_
