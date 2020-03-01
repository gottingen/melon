//
// Created by liyinbin on 2020/2/8.
//


#include <abel/metrics/histogram.h>
#include <abel/metrics/metrics_type.h>
#include <algorithm>
#include <cassert>
#include <iterator>
#include <numeric>

namespace abel {
    namespace metrics {

        histogram::histogram(const bucket &buckets) noexcept
                : _bucket_boundaries{buckets}, _bucket_counts{buckets.size() + 1}, _sum{} {
            assert(std::is_sorted(std::begin(_bucket_boundaries),
                                  std::end(_bucket_boundaries)));
        }

        void histogram::observe(const double value) noexcept {
            // TODO: determine bucket list size at which binary search would be faster
            const auto bucket_index = static_cast<std::size_t>(std::distance(
                    _bucket_boundaries.begin(),
                    std::find_if(
                            std::begin(_bucket_boundaries), std::end(_bucket_boundaries),
                            [value](const double boundary) { return boundary >= value; })));
            _sum.inc(value);
            _bucket_counts[bucket_index].inc();
        }

        cache_metrics histogram::collect() const noexcept {
            auto metric = cache_metrics{};

            metric.type = metrics_type::mt_histogram;

            auto cumulative_count = 0ULL;
            for (std::size_t i{0}; i < _bucket_counts.size(); ++i) {
                cumulative_count += _bucket_counts[i].value();
                auto bucket = cache_metrics::cached_bucket{};
                bucket.cumulative_count = cumulative_count;
                bucket.upper_bound = (i == _bucket_boundaries.size()
                                      ? std::numeric_limits<double>::infinity()
                                      : _bucket_boundaries[i]);
                metric.histogram.bucket.push_back(std::move(bucket));
            }
            metric.histogram.sample_count = cumulative_count;
            metric.histogram.sample_sum = _sum.value();

            return metric;
        }

    } //namespace metrics
} //namespace abel
