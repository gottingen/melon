//
// Created by liyinbin on 2020/2/8.
//


#include <abel/metrics/timer.h>
#include <abel/chrono/clock.h>

namespace abel {
namespace metrics {

timer::timer (const bucket &buckets)
    : _bucket_boundaries {buckets}, _bucket_counts {buckets.size() + 1}, _sum {} {
    assert(std::is_sorted(std::begin(_bucket_boundaries),
                          std::end(_bucket_boundaries)));
}

std::shared_ptr <timer> timer::new_timer (const bucket &buckets) {
    std::shared_ptr <timer> tr(new timer(buckets));
    return tr;
}

void timer::observe (abel::duration d) {

    auto value = abel::to_double_microseconds(d);
    const auto bucket_index = static_cast<std::size_t>(std::distance(
        _bucket_boundaries.begin(),
        std::find_if(
            std::begin(_bucket_boundaries), std::end(_bucket_boundaries),
            [value] (const double boundary) { return boundary >= value; })));
    _sum.inc(value);
    _bucket_counts[bucket_index].inc();

}

void timer::observe (int64_t tick) {
    auto d = abel::microseconds(tick);
    observe(d);
}

stop_watcher timer::start () {
    return stop_watcher(abel::now(), shared_from_this());
}

void timer::record (abel::abel_time start) {
    auto const duration = abel::now() - start;
    observe(duration);
}

cache_metrics timer::collect () const noexcept {
    auto metric = cache_metrics {};

    metric.type = metrics_type::mt_timer;

    auto cumulative_count = 0ULL;
    for (std::size_t i {0}; i < _bucket_counts.size(); ++i) {
        cumulative_count += _bucket_counts[i].value();
        auto bucket = cache_metrics::cached_bucket {};
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
