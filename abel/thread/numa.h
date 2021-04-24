//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_THREAD_NUMA_H_
#define ABEL_THREAD_NUMA_H_


#include <cinttypes>
#include <optional>
#include <string>
#include <vector>

#include "abel/hash/hash.h"
#include "abel/container/inlined_vector.h"
#include "abel/thread/affinity.h"

namespace abel {

    namespace numa {
        struct numa_node {
            int id;
            std::vector<int> logical_cpus;
        };

        inline bool support_affinity() {
#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__)) || \
    defined(__FreeBSD__)
            static constexpr bool supported = true;
#else
            static constexpr bool supported = false;
#endif
            return supported;
        }

        std::vector<numa_node> get_available_nodes();

        int get_current_node();

        std::size_t get_current_node_index();

        int get_node_id(std::size_t index);

        std::size_t get_node_index(int node_id);

        std::size_t get_number_of_nodes_available();

        int get_node_of_processor(int cpu);

    }  // namespace numa

    int get_current_processor_id();

    std::size_t GetNumberOfProcessorsAvailable();

    std::size_t GetNumberOfProcessorsConfigured();

    bool IsInaccessibleProcessorPresent();

    bool IsProcessorAccessible(int cpu);

    // Parse processor list. e.g., "1-10,21,-1".
    //
    // Not sure if this is the right place to declare it though..
    std::optional<std::vector<int>> TryParseProcesserList(const std::string &s);

    // Some helper methods for manipulating threads.

    // Set affinity of the calling thread.
    //
    // Returns error number, or 0 on success.
    int TrySetCurrentThreadAffinity(const std::vector<int>& affinity);

    // Set affinity of the calling thread.
    //
    // Abort on failure.
    void SetCurrentThreadAffinity(const std::vector<int>& affinity);

    // Get affinity of the calling thread.
    std::vector<int> GetCurrentThreadAffinity();

    // Set name of the calling thread.
    //
    // Error, if any, is ignored.
    void SetCurrentThreadName(const std::string& name);

}  // namespace abel

#endif  // ABEL_THREAD_NUMA_H_
