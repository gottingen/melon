// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_METRICS_HISTOGRAM_H_
#define ABEL_METRICS_HISTOGRAM_H_

#include <vector>
#include <memory>
#include "abel/metrics/counter.h"
#include "abel/metrics/cache_metrics.h"
#include "abel/metrics/bucket.h"

namespace abel {
namespace metrics {

class histogram {
  public:

    histogram(const bucket &buckets) noexcept;

    void observe(double value) noexcept;

    cache_metrics collect() const noexcept;

  private:
    const bucket _bucket_boundaries;
    std::vector<counter> _bucket_counts;
    counter _sum;
};

typedef std::shared_ptr<histogram> histogram_ptr;
}  // namespace metrics
}  // namespace abel
#endif  // ABEL_ABEL_METRICS_HISTOGRAM_H_
