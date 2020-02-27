//

#include <abel/strings/escaping.h>

#include <cstdio>
#include <cstring>
#include <random>

#include <benchmark/benchmark.h>
#include <abel/log/raw_logging.h>
#include <test/testing/escaping_test_common.h>

namespace {

    void BM_CUnescapeHexString(benchmark::State &state) {
        std::string src;
        for (int i = 0; i < 50; i++) {
            src += "\\x55";
        }
        std::string dest;
        for (auto _ : state) {
            abel::unescape(src, &dest);
        }
    }

    BENCHMARK(BM_CUnescapeHexString);

    void BM_WebSafeBase64Escape_string(benchmark::State &state) {
        std::string raw;
        for (int i = 0; i < 10; ++i) {
            for (const auto &test_set : abel::strings_internal::base64_strings()) {
                raw += std::string(test_set.plaintext);
            }
        }

        // The actual benchmark loop is tiny...
        std::string escaped;
        for (auto _ : state) {
            abel::web_safe_base64_escape(raw, &escaped);
        }

        // We want to be sure the compiler doesn't throw away the loop above,
        // and the easiest way to ensure that is to round-trip the results and verify
        // them.
        std::string round_trip;
        abel::web_safe_base64_unescape(escaped, &round_trip);
        ABEL_RAW_CHECK(round_trip == raw, "");
    }

    BENCHMARK(BM_WebSafeBase64Escape_string);

// Used for the escape benchmarks
    const char kStringValueNoEscape[] = "1234567890";
    const char kStringValueSomeEscaped[] = "123\n56789\xA1";
    const char kStringValueMostEscaped[] = "\xA1\xA2\ny\xA4\xA5\xA6z\b\r";

    void CEscapeBenchmarkHelper(benchmark::State &state, const char *string_value,
                                int max_len) {
        std::string src;
        while (src.size() < static_cast<size_t>(max_len)) {
            abel::string_append(&src, string_value);
        }

        for (auto _ : state) {
            abel::escape(src);
        }
    }

    void BM_CEscape_NoEscape(benchmark::State &state) {
        CEscapeBenchmarkHelper(state, kStringValueNoEscape, state.range(0));
    }

    BENCHMARK(BM_CEscape_NoEscape)
    ->Range(1, 1 << 14);

    void BM_CEscape_SomeEscaped(benchmark::State &state) {
        CEscapeBenchmarkHelper(state, kStringValueSomeEscaped, state.range(0));
    }

    BENCHMARK(BM_CEscape_SomeEscaped)
    ->Range(1, 1 << 14);

    void BM_CEscape_MostEscaped(benchmark::State &state) {
        CEscapeBenchmarkHelper(state, kStringValueMostEscaped, state.range(0));
    }

    BENCHMARK(BM_CEscape_MostEscaped)
    ->Range(1, 1 << 14);

}  // namespace
