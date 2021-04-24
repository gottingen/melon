// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_METRICS_PROM_SERIALIZER_H_
#define ABEL_METRICS_PROM_SERIALIZER_H_

#include <iosfwd>
#include <string>
#include <vector>
#include "abel/metrics/cache_metrics.h"
#include "abel/metrics/serializer.h"

namespace abel {
namespace metrics {

class prometheus_serializer : public serializer {
  public:
    using serializer::serializer;

    void format(std::ostream &stdout, const std::vector<cache_metrics> &) const override;
};

}  // namespace metrics
}  // namespace abel

#endif  // ABEL_METRICS_PROM_SERIALIZER_H_
