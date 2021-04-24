//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_RUNTIME_H_
#define ABEL_FIBER_RUNTIME_H_


#include <cstdlib>

#include "abel/base/annotation.h"
#include "abel/base/profile.h"

namespace abel {

    namespace fiber_internal {

        std::size_t get_current_scheduling_groupIndex_slow();

    }  // namespace fiber_internal

    // Bring the whole world up.
    //
    // All stuffs about fiber are initialized by this method.
    void start_runtime();

    // Bring the whole world down.
    void terminate_runtime();

    // Get number of scheduling groups started.
    std::size_t get_scheduling_group_count();

    // Get the scheduling group the caller thread / fiber is currently belonging to.
    //
    // Calling this method outside of any scheduling group is undefined.
    inline std::size_t get_current_scheduling_group_index() {
        ABEL_INTERNAL_TLS_MODEL thread_local std::size_t index =
                fiber_internal::get_current_scheduling_groupIndex_slow();
        return index;
    }

    // Get the scheduling group size.
    std::size_t get_scheduling_group_size();

    // Get NUMA node assigned to a given scheduling group. This method only makes
    // sense if NUMA aware is enabled. Otherwise 0 is returned.
    int get_scheduling_group_assigned_node(std::size_t sg_index);

    namespace fiber_internal {

        class scheduling_group;

        // Find scheduling group by ID.
        //
        // Precondition: `index` < `get_scheduling_group_count().`
        scheduling_group* routine_get_scheduling_group(std::size_t index);

        // Get scheduling group "nearest" to the calling thread.
        //
        // - If calling thread is a fiber worker, it's current scheduling group is
        //   returned.
        //
        // - Otherwise if NUMA aware is enabled, a randomly chosen scheduling group in
        //   the same node is returned.
        //
        // - If no scheduling group is initialized in current node, or NUMA aware is not
        //   enabled, a randomly chosen one is returned.
        //
        // - If no scheduling group is initialize at all, `nullptr` is returned instead.
        scheduling_group* nearest_scheduling_group_slow(scheduling_group** cache);

        inline scheduling_group* nearest_scheduling_group() {
            ABEL_INTERNAL_TLS_MODEL thread_local scheduling_group* nearest{};
            if (ABEL_LIKELY(nearest)) {
                return nearest;
            }
            return nearest_scheduling_group_slow(&nearest);
        }

        // Same as `nearest_scheduling_group()`, but this one returns an index instead.
        //
        // Returns -1 if no scheduling group is initialized at all.
        std::ptrdiff_t nearest_scheduling_group_index();

    }  // namespace fiber_internal

}  // namespace abel


#endif  // ABEL_FIBER_RUNTIME_H_
