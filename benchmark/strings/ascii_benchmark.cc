//

#include <abel/strings/ascii.h>
#include <abel/strings/case_conv.h>
#include <cctype>
#include <string>
#include <array>
#include <random>

#include <benchmark/benchmark.h>

namespace {

    std::array<unsigned char, 256> MakeShuffledBytes() {
        std::array<unsigned char, 256> bytes;
        for (size_t i = 0; i < 256; ++i) bytes[i] = static_cast<unsigned char>(i);
        std::random_device rd;
        std::seed_seq seed({rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()});
        std::mt19937 g(seed);
        std::shuffle(bytes.begin(), bytes.end(), g);
        return bytes;
    }

    template<typename Function>
    void AsciiBenchmark(benchmark::State &state, Function f) {
        std::array<unsigned char, 256> bytes = MakeShuffledBytes();
        size_t sum = 0;
        for (auto _ : state) {
            for (unsigned char b : bytes) sum += f(b) ? 1 : 0;
        }
        // Make a copy of `sum` before calling `DoNotOptimize` to make sure that `sum`
        // can be put in a CPU register and not degrade performance in the loop above.
        size_t sum2 = sum;
        benchmark::DoNotOptimize(sum2);
        state.SetBytesProcessed(state.iterations() * bytes.size());
    }

    using StdAsciiFunction = int (*)(int);

    template<StdAsciiFunction f>
    void BM_Ascii(benchmark::State &state) {
        AsciiBenchmark(state, f);
    }

    using AbelAsciiIsFunction = bool (*)(unsigned char);

    template<AbelAsciiIsFunction f>
    void BM_Ascii(benchmark::State &state) {
        AsciiBenchmark(state, f);
    }

    using AbelAsciiToFunction = char (*)(unsigned char);

    template<AbelAsciiToFunction f>
    void BM_Ascii(benchmark::State &state) {
        AsciiBenchmark(state, f);
    }

    inline char Noop(unsigned char b) { return static_cast<char>(b); }

    BENCHMARK_TEMPLATE(BM_Ascii, Noop
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::isalpha
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_alpha
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::isdigit
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_digit
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::isalnum
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_alpha_numeric
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::isspace
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_space
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::ispunct
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_punct
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::isblank
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_white
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::iscntrl
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_control
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::isxdigit
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_hex_digit
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::isprint
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_print
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::isgraph
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_graph
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::isupper
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_upper
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::islower
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_lower
    );
    BENCHMARK_TEMPLATE(BM_Ascii, isascii
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::is_ascii
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::tolower
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::to_lower
    );
    BENCHMARK_TEMPLATE(BM_Ascii, std::toupper
    );
    BENCHMARK_TEMPLATE(BM_Ascii, abel::ascii::to_upper
    );

    static void BM_StrToLower(benchmark::State &state) {
        const int size = state.range(0);
        std::string s(size, 'X');
        for (auto _ : state) {
            benchmark::DoNotOptimize(abel::string_to_lower(s));
        }
    }

    BENCHMARK(BM_StrToLower)
    ->Range(1, 1 << 20);

    static void BM_StrToUpper(benchmark::State &state) {
        const int size = state.range(0);
        std::string s(size, 'x');
        for (auto _ : state) {
            benchmark::DoNotOptimize(abel::string_to_upper(s));
        }
    }

    BENCHMARK(BM_StrToUpper)
    ->Range(1, 1 << 20);

}  // namespace
