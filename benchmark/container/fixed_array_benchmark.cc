//

#include <stddef.h>

#include <string>

#include <benchmark/benchmark.h>
#include <abel/container/fixed_array.h>

namespace {

// For benchmarking -- simple class with constructor and destructor that
// set an int to a constant..
    class SimpleClass {
    public:
        SimpleClass() : i(3) {}

        ~SimpleClass() { i = 0; }

    private:
        int i;
    };

    template<typename C, size_t stack_size>
    void BM_FixedArray(benchmark::State &state) {
        const int size = state.range(0);
        for (auto _ : state) {
            abel::fixed_array<C, stack_size> fa(size);
            benchmark::DoNotOptimize(fa.data());
        }
    }

    BENCHMARK_TEMPLATE(BM_FixedArray,
    char, abel::kFixedArrayUseDefault)
    ->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray,
    char, 0)->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray,
    char, 1)->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray,
    char, 16)->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray,
    char, 256)->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray,
    char, 65536)->Range(0, 1 << 16);

    BENCHMARK_TEMPLATE(BM_FixedArray, SimpleClass, abel::kFixedArrayUseDefault
    )
    ->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray, SimpleClass,
    0)->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray, SimpleClass,
    1)->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray, SimpleClass,
    16)->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray, SimpleClass,
    256)->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray, SimpleClass,
    65536)->Range(0, 1 << 16);

    BENCHMARK_TEMPLATE(BM_FixedArray, std::string, abel::kFixedArrayUseDefault
    )
    ->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray, std::string,
    0)->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray, std::string,
    1)->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray, std::string,
    16)->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray, std::string,
    256)->Range(0, 1 << 16);
    BENCHMARK_TEMPLATE(BM_FixedArray, std::string,
    65536)->Range(0, 1 << 16);

}  // namespace
