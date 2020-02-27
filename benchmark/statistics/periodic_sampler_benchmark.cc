//

#include <benchmark/benchmark.h>
#include <abel/statistics/periodic_sampler.h>

namespace abel {

        namespace {

            template<typename Sampler>
            void BM_Sample(Sampler *sampler, benchmark::State &state) {
                for (auto _ : state) {
                    benchmark::DoNotOptimize(sampler);
                    benchmark::DoNotOptimize(sampler->sample());
                }
            }

            template<typename Sampler>
            void BM_SampleMinunumInlined(Sampler *sampler, benchmark::State &state) {
                for (auto _ : state) {
                    benchmark::DoNotOptimize(sampler);
                    if (ABEL_UNLIKELY(sampler->subtle_maybe_sample())) {
                        benchmark::DoNotOptimize(sampler->subtle_confirm_sample());
                    }
                }
            }

            void BM_PeriodicSampler_TinySample(benchmark::State &state) {
                struct Tag {
                };
                periodic_sampler<Tag, 10> sampler;
                BM_Sample(&sampler, state);
            }

            BENCHMARK(BM_PeriodicSampler_TinySample);

            void BM_PeriodicSampler_ShortSample(benchmark::State &state) {
                struct Tag {
                };
                periodic_sampler<Tag, 1024> sampler;
                BM_Sample(&sampler, state);
            }

            BENCHMARK(BM_PeriodicSampler_ShortSample);

            void BM_PeriodicSampler_LongSample(benchmark::State &state) {
                struct Tag {
                };
                periodic_sampler<Tag, 1024 * 1024> sampler;
                BM_Sample(&sampler, state);
            }

            BENCHMARK(BM_PeriodicSampler_LongSample);

            void BM_PeriodicSampler_LongSampleMinunumInlined(benchmark::State &state) {
                struct Tag {
                };
                periodic_sampler<Tag, 1024 * 1024> sampler;
                BM_SampleMinunumInlined(&sampler, state);
            }

            BENCHMARK(BM_PeriodicSampler_LongSampleMinunumInlined);

            void BM_PeriodicSampler_Disabled(benchmark::State &state) {
                struct Tag {
                };
                periodic_sampler<Tag, 0> sampler;
                BM_Sample(&sampler, state);
            }

            BENCHMARK(BM_PeriodicSampler_Disabled);

        }  // namespace

}  // namespace abel
