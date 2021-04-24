// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_ABEL_METRICS_COUNTER_H_
#define ABEL_ABEL_METRICS_COUNTER_H_

#include <memory>
#include "abel/metrics/gauge.h"
#include "abel/metrics/cache_metrics.h"

namespace abel {
namespace metrics {

class counter {
  public:

    counter() = default;

    void inc();

    void inc(double);

    double value() const;

    cache_metrics collect() const noexcept;

  private:
    gauge _gauge{0.0};
};

typedef std::shared_ptr<counter> counter_ptr;

}  // namespace ,metrics
}  // namespace abel

#endif  // ABEL_ABEL_METRICS_COUNTER_H_
