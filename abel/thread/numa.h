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
// For the moment I'm not very satisfied about interfaces here, so I placed this
// header in `internal/`.

namespace abel {

    namespace numa {
        struct numa_node {
            int id;
            std::vector<int> logical_cpus;
        };

        std::vector<numa_node> GetAvailableNodes();

        int GetCurrentNode();

        std::size_t GetCurrentNodeIndex();

        int GetNodeId(std::size_t index);

        std::size_t GetNodeIndex(int node_id);

        std::size_t GetNumberOfNodesAvailable();

        int GetNodeOfProcessor(int cpu);

    }  // namespace numa

    int GetCurrentProcessorId();

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
