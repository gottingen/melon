//

#include <abel/strings/internal/char_map.h>

#include <cstdint>

#include <benchmark/benchmark.h>

namespace {

    abel::strings_internal::Charmap MakeBenchmarkMap() {
        abel::strings_internal::Charmap m;
        uint32_t x[] = {0x0, 0x1, 0x2, 0x3, 0xf, 0xe, 0xd, 0xc};
        for (uint32_t &t : x) t *= static_cast<uint32_t>(0x11111111UL);
        for (uint32_t i = 0; i < 256; ++i) {
            if ((x[i / 32] >> (i % 32)) & 1)
                m = m | abel::strings_internal::Charmap::Char(i);
        }
        return m;
    }

// Micro-benchmark for Charmap::contains.
    void BM_Contains(benchmark::State &state) {
        // Loop-body replicated 10 times to increase time per iteration.
        // Argument continuously changed to avoid generating common subexpressions.
        const abel::strings_internal::Charmap benchmark_map = MakeBenchmarkMap();
        unsigned char c = 0;
        int ops = 0;
        for (auto _ : state) {
            ops += benchmark_map.contains(c++);
            ops += benchmark_map.contains(c++);
            ops += benchmark_map.contains(c++);
            ops += benchmark_map.contains(c++);
            ops += benchmark_map.contains(c++);
            ops += benchmark_map.contains(c++);
            ops += benchmark_map.contains(c++);
            ops += benchmark_map.contains(c++);
            ops += benchmark_map.contains(c++);
            ops += benchmark_map.contains(c++);
        }
        benchmark::DoNotOptimize(ops);
    }

    BENCHMARK(BM_Contains);

// We don't bother benchmarking Charmap::IsZero or Charmap::IntersectsWith;
// their running time is data-dependent and it is not worth characterizing
// "typical" data.

}  // namespace
