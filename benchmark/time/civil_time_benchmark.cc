//

#include <abel/chrono/civil_time.h>
#include <numeric>
#include <vector>
#include <abel/hash/hash.h>
#include <benchmark/benchmark.h>

namespace {

// Run on (12 X 3492 MHz CPUs); 2018-11-05T13:44:29.814239103-08:00
// CPU: Intel Haswell with HyperThreading (6 cores) dL1:32KB dL2:256KB dL3:15MB
// Benchmark                 abel_time(ns)        CPU(ns)     Iterations
// ----------------------------------------------------------------
// BM_Difference_Days              14.5           14.5     48531105
// BM_Step_Days                    12.6           12.6     54876006
// BM_Format                      587            587        1000000
// BM_Parse                       692            692        1000000
// BM_RoundTripFormatParse       1309           1309         532075
// BM_CivilYearAbelHash             0.710          0.710  976400000
// BM_CivilMonthAbelHash            1.13           1.13   619500000
// BM_CivilDayAbelHash              1.70           1.70   426000000
// BM_CivilHourAbelHash             2.45           2.45   287600000
// BM_CivilMinuteAbelHash           3.21           3.21   226200000
// BM_CivilSecondAbelHash           4.10           4.10   171800000

    void BM_Difference_Days(benchmark::State &state) {
        const abel::chrono_day c(2014, 8, 22);
        const abel::chrono_day epoch(1970, 1, 1);
        while (state.KeepRunning()) {
            const abel::chrono_diff_t n = c - epoch;
            benchmark::DoNotOptimize(n);
        }
    }

    BENCHMARK(BM_Difference_Days);

    void BM_Step_Days(benchmark::State &state) {
        const abel::chrono_day kStart(2014, 8, 22);
        abel::chrono_day c = kStart;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(++c);
        }
    }

    BENCHMARK(BM_Step_Days);

    void BM_Format(benchmark::State &state) {
        const abel::chrono_second c(2014, 1, 2, 3, 4, 5);
        while (state.KeepRunning()) {
            const std::string s = abel::format_chrono_time(c);
            benchmark::DoNotOptimize(s);
        }
    }

    BENCHMARK(BM_Format);

    void BM_Parse(benchmark::State &state) {
        const std::string f = "2014-01-02T03:04:05";
        abel::chrono_second c;
        while (state.KeepRunning()) {
            const bool b = abel::parse_chrono_time(f, &c);
            benchmark::DoNotOptimize(b);
        }
    }

    BENCHMARK(BM_Parse);

    void BM_RoundTripFormatParse(benchmark::State &state) {
        const abel::chrono_second c(2014, 1, 2, 3, 4, 5);
        abel::chrono_second out;
        while (state.KeepRunning()) {
            const bool b = abel::parse_chrono_time(abel::format_chrono_time(c), &out);
            benchmark::DoNotOptimize(b);
        }
    }

    BENCHMARK(BM_RoundTripFormatParse);

    template<typename T>
    void BM_CivilTimeAbelHash(benchmark::State &state) {
        const int kSize = 100000;
        std::vector<T> civil_times(kSize);
        std::iota(civil_times.begin(), civil_times.end(), T(2018));

        abel::hash<T> abel_hasher;
        while (state.KeepRunningBatch(kSize)) {
            for (const T civil_time : civil_times) {
                benchmark::DoNotOptimize(abel_hasher(civil_time));
            }
        }
    }

    void BM_CivilYearAbelHash(benchmark::State &state) {
        BM_CivilTimeAbelHash<abel::chrono_year>(state);
    }

    void BM_CivilMonthAbelHash(benchmark::State &state) {
        BM_CivilTimeAbelHash<abel::chrono_month>(state);
    }

    void BM_CivilDayAbelHash(benchmark::State &state) {
        BM_CivilTimeAbelHash<abel::chrono_day>(state);
    }

    void BM_CivilHourAbelHash(benchmark::State &state) {
        BM_CivilTimeAbelHash<abel::chrono_hour>(state);
    }

    void BM_CivilMinuteAbelHash(benchmark::State &state) {
        BM_CivilTimeAbelHash<abel::chrono_minute>(state);
    }

    void BM_CivilSecondAbelHash(benchmark::State &state) {
        BM_CivilTimeAbelHash<abel::chrono_second>(state);
    }

    BENCHMARK(BM_CivilYearAbelHash);
    BENCHMARK(BM_CivilMonthAbelHash);
    BENCHMARK(BM_CivilDayAbelHash);
    BENCHMARK(BM_CivilHourAbelHash);
    BENCHMARK(BM_CivilMinuteAbelHash);
    BENCHMARK(BM_CivilSecondAbelHash);

}  // namespace
