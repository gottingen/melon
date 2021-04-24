// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_ABEL_METRICS_TIMER_H_
#define ABEL_ABEL_METRICS_TIMER_H_

#include <memory>
#include <vector>
#include "abel/metrics/stop_watcher.h"
#include "abel/metrics/counter.h"
#include "abel/metrics/bucket.h"
#include "abel/metrics/cache_metrics.h"

namespace abel {
namespace metrics {

class timer : public std::enable_shared_from_this<timer> {
  public:
    void observe(abel::duration);

    void observe(int64_t);

    stop_watcher start();

    cache_metrics collect() const noexcept;

    static std::shared_ptr<timer> new_timer(const bucket &buckets);

    void record(abel::time_point start);

  private:
    timer(const bucket &buckets);

  private:
    const bucket _bucket_boundaries;
    std::vector<counter> _bucket_counts;
    counter _sum;
};

typedef std::shared_ptr<timer> timer_ptr;

}  // namespace metrics
}  // namespace abel

#endif  // ABEL_ABEL_METRICS_TIMER_H_
