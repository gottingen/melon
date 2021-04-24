//
// Created by liyinbin on 2021/4/4.
//

#ifndef ABEL_FIBER_INTERNAL_SCHEDULING_PARAMETERS_H_
#define ABEL_FIBER_INTERNAL_SCHEDULING_PARAMETERS_H_

#include <cstddef>

namespace abel {
    namespace fiber_internal {
        enum class SchedulingProfile {
            // Use this profile if your workload (running in fiber) tends to run long
            // (tens or hundres of milliseconds) without yielding fiber worker.
            //
            // For such use cases, it's important to share CPUs between fibers as much as
            // possible to avoid starvation, even at the cost of framework-internal
            // contention or sacrificing NUMA-locality.
            //
            // This profile:
            //
            // - Groups as many fiber workers as possible into a single work group.
            // - DISABLES NUMA awareness for fiber scheduling (but not object pool, etc.).
            ComputeHeavy,

            // Not as aggressive as `ComputeHeavy`. This profile prefers a large
            // scheduling group while still respect NUMA topology.
            //
            // This profile:
            //
            // - Enables NUMA awareness if requested concurrency is greater than half of
            //   available processors.
            // - So long as NUMA topology is respected, groups as many workers as possible
            //   into a single work group.
            Compute,

            // This profile tries to find a balance between reducing framework-internal
            // contention and encouraging sharing CPUs between fiber workers.
            //
            // This profile:
            //
            // - Uses a scheduling-group size between [16, 32).
            // - Enables NUMA awareness if (requested concurrency / number of NUMA nodes)
            //   results in a per-node-concurrency that fits in (or a multiple of) the
            //   scheduling-group size specification above.
            Neutral,

            // Use this profile if your workload tends to be quick, or yields a lot.
            //
            // For such use cases, we can use a smaller scheduling group to reduce
            // framework-internal contention, without risking starving fibers in run queue
            // for too long.
            //
            // This profile is the same as `Neutral` except that:
            //
            // - Uses a scheduling-group size between [12, 20).
            Io,

            // This profile prefers a smaller scheduling group, and is otherwise the same
            // as `Io`.
            //
            // This profile is the same as `Neutral` except that:
            //
            // - Uses a scheduling group size between [8, 16).
            IoHeavy,
        };

        struct SchedulingParameters {
            std::size_t scheduling_groups;
            std::size_t workers_per_group;

            // Possibly set only if scheduling groups can be distributed into NUMA domains
            // evenly.
            bool enable_numa_affinity;
        };

// Determines scheduling parameters based on desired concurrency and profile.
        SchedulingParameters GetSchedulingParameters(SchedulingProfile profile,
                                                     std::size_t numa_domains,
                                                     std::size_t available_processors,
                                                     std::size_t desired_concurrency);

    }  // namespace fiber_internal
}  // namespace abel
#endif  // ABEL_FIBER_INTERNAL_SCHEDULING_PARAMETERS_H_
