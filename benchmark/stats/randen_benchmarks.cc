//
#include <abel/stats/random/engine/randen.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <abel/base/profile.h>
#include <abel/log/abel_logging.h>
#include <test/testing/nanobenchmark.h>
#include <abel/stats/random/engine/randen_engine.h>
#include <abel/stats/random/engine/randen_hwaes.h>
#include <abel/stats/random/engine/randen_slow.h>
#include <abel/strings/numbers.h>

namespace {

    using abel::random_internal::randen;
    using abel::random_internal::randen_hw_aes;
    using abel::random_internal::randen_slow;

    using abel::random_internal_nanobenchmark::FuncInput;
    using abel::random_internal_nanobenchmark::FuncOutput;
    using abel::random_internal_nanobenchmark::invariant_ticks_per_second;
    using abel::random_internal_nanobenchmark::measure_closure;
    using abel::random_internal_nanobenchmark::Params;
    using abel::random_internal_nanobenchmark::pin_thread_to_cpu;
    using abel::random_internal_nanobenchmark::Result;

// Local state parameters.
    static constexpr size_t kStateSizeT = randen::kStateBytes / sizeof(uint64_t);
    static constexpr size_t kSeedSizeT = randen::kSeedBytes / sizeof(uint32_t);

// randen implementation benchmarks.
    template<typename T>
    struct AbsorbFn : public T {
        mutable uint64_t state[kStateSizeT] = {};
        mutable uint32_t seed[kSeedSizeT] = {};

        static constexpr size_t bytes() { return sizeof(seed); }

        FuncOutput operator()(const FuncInput num_iters) const {
            for (size_t i = 0; i < num_iters; ++i) {
                this->absorb(seed, state);
            }
            return state[0];
        }
    };

    template<typename T>
    struct GenerateFn : public T {
        mutable uint64_t state[kStateSizeT];

        GenerateFn() { std::memset(state, 0, sizeof(state)); }

        static constexpr size_t bytes() { return sizeof(state); }

        FuncOutput operator()(const FuncInput num_iters) const {
            const auto *keys = this->get_keys();
            for (size_t i = 0; i < num_iters; ++i) {
                this->generate(keys, state);
            }
            return state[0];
        }
    };

    template<typename UInt>
    struct Engine {
        mutable abel::random_internal::randen_engine<UInt> rng;

        static constexpr size_t bytes() { return sizeof(UInt); }

        FuncOutput operator()(const FuncInput num_iters) const {
            for (size_t i = 0; i < num_iters - 1; ++i) {
                rng();
            }
            return rng();
        }
    };

    template<size_t N>
    void Print(const char *name, const size_t n, const Result (&results)[N],
               const size_t bytes) {
        if (n == 0) {
            ABEL_RAW_LOG(
                    WARNING,
                    "WARNING: Measurement failed, should not happen when using "
                    "pin_thread_to_cpu unless the region to measure takes > 1 second.\n");
            return;
        }

        static const double ns_per_tick = 1e9 / invariant_ticks_per_second();
        static constexpr const double kNsPerS = 1e9;                 // ns/s
        static constexpr const double kMBPerByte = 1.0 / 1048576.0;  // Mb / b
        static auto header = [] {
            return printf("%20s %8s: %12s ticks; %9s  (%9s) %8s\n", "Name", "Count",
                          "Total", "Variance", "abel_time", "bytes/s");
        }();
        (void) header;

        for (size_t i = 0; i < n; ++i) {
            const double ticks_per_call = results[i].ticks / results[i].input;
            const double ns_per_call = ns_per_tick * ticks_per_call;
            const double bytes_per_ns = bytes / ns_per_call;
            const double mb_per_s = bytes_per_ns * kNsPerS * kMBPerByte;
            // Output
            printf("%20s %8zu: %12.2f ticks; MAD=%4.2f%%  (%6.1f ns) %8.1f Mb/s\n",
                   name, results[i].input, results[i].ticks,
                   results[i].variability * 100.0, ns_per_call, mb_per_s);
        }
    }

// Fails here
    template<typename Op, size_t N>
    void measure(const char *name, const FuncInput (&inputs)[N]) {
        Op op;

        Result results[N];
        Params params;
        params.verbose = false;
        params.max_evals = 6;  // avoid test timeout
        const size_t num_results = measure_closure(op, inputs, N, results, params);
        Print(name, num_results, results, op.bytes());
    }

// unpredictable == 1 but the compiler does not know that.
    void RunAll(const int argc, char *argv[]) {
        if (argc == 2) {
            int cpu = -1;
            if (!abel::simple_atoi(argv[1], &cpu)) {
                ABEL_RAW_LOG(FATAL, "The optional argument must be a CPU number >= 0.\n");
            }
            pin_thread_to_cpu(cpu);
        }

        // The compiler cannot reduce this to a constant.
        const FuncInput unpredictable = (argc != 999);
        static const FuncInput inputs[] = {unpredictable * 100, unpredictable * 1000};

#if !defined(ABEL_INTERNAL_DISABLE_AES) && ABEL_HAVE_ACCELERATED_AES
        measure<AbsorbFn<randen_hw_aes>>("Absorb (HwAes)", inputs);
#endif
        measure<AbsorbFn<randen_slow>>("Absorb (Slow)", inputs);

#if !defined(ABEL_INTERNAL_DISABLE_AES) && ABEL_HAVE_ACCELERATED_AES
        measure<GenerateFn<randen_hw_aes>>("Generate (HwAes)", inputs);
#endif
        measure<GenerateFn<randen_slow>>("Generate (Slow)", inputs);

        // measure the production engine.
        static const FuncInput inputs1[] = {unpredictable * 1000,
                                            unpredictable * 10000};
        measure<Engine<uint64_t>>("randen_engine<uint64_t>", inputs1);
        measure<Engine<uint32_t>>("randen_engine<uint32_t>", inputs1);
    }

}  // namespace

int main(int argc, char *argv[]) {
    RunAll(argc, argv);
    return 0;
}
