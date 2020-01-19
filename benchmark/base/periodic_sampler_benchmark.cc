//

#include <benchmark/benchmark.h>
#include <abel/base/internal/periodic_sampler.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace base_internal {
namespace {

template <typename Sampler>
void BM_Sample(Sampler* sampler, benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(sampler);
    benchmark::DoNotOptimize(sampler->Sample());
  }
}

template <typename Sampler>
void BM_SampleMinunumInlined(Sampler* sampler, benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(sampler);
    if (ABEL_UNLIKELY(sampler->SubtleMaybeSample())) {
      benchmark::DoNotOptimize(sampler->SubtleConfirmSample());
    }
  }
}

void BM_PeriodicSampler_TinySample(benchmark::State& state) {
  struct Tag {};
  PeriodicSampler<Tag, 10> sampler;
  BM_Sample(&sampler, state);
}
BENCHMARK(BM_PeriodicSampler_TinySample);

void BM_PeriodicSampler_ShortSample(benchmark::State& state) {
  struct Tag {};
  PeriodicSampler<Tag, 1024> sampler;
  BM_Sample(&sampler, state);
}
BENCHMARK(BM_PeriodicSampler_ShortSample);

void BM_PeriodicSampler_LongSample(benchmark::State& state) {
  struct Tag {};
  PeriodicSampler<Tag, 1024 * 1024> sampler;
  BM_Sample(&sampler, state);
}
BENCHMARK(BM_PeriodicSampler_LongSample);

void BM_PeriodicSampler_LongSampleMinunumInlined(benchmark::State& state) {
  struct Tag {};
  PeriodicSampler<Tag, 1024 * 1024> sampler;
  BM_SampleMinunumInlined(&sampler, state);
}
BENCHMARK(BM_PeriodicSampler_LongSampleMinunumInlined);

void BM_PeriodicSampler_Disabled(benchmark::State& state) {
  struct Tag {};
  PeriodicSampler<Tag, 0> sampler;
  BM_Sample(&sampler, state);
}
BENCHMARK(BM_PeriodicSampler_Disabled);

}  // namespace
}  // namespace base_internal
ABEL_NAMESPACE_END
}  // namespace abel
