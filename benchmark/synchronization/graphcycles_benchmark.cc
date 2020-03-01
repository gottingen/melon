//

#include <abel/synchronization/internal/graphcycles.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include <benchmark/benchmark.h>
#include <abel/log/raw_logging.h>

namespace {

    void BM_StressTest(benchmark::State &state) {
        const int num_nodes = state.range(0);
        while (state.KeepRunningBatch(num_nodes)) {
            abel::synchronization_internal::GraphCycles g;
            std::vector<abel::synchronization_internal::GraphId> nodes(num_nodes);
            for (int i = 0; i < num_nodes; i++) {
                nodes[i] = g.GetId(reinterpret_cast<void *>(static_cast<uintptr_t>(i)));
            }
            for (int i = 0; i < num_nodes; i++) {
                int end = std::min(num_nodes, i + 5);
                for (int j = i + 1; j < end; j++) {
                    ABEL_RAW_CHECK(g.InsertEdge(nodes[i], nodes[j]), "");
                }
            }
        }
    }

    BENCHMARK(BM_StressTest)
    ->Range(2048, 1048576);

}  // namespace
