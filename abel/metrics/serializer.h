// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_ABEL_METRICS_SERIALIZER_H_
#define ABEL_ABEL_METRICS_SERIALIZER_H_
#include <iosfwd>
#include <string>
#include <vector>

#include "abel/metrics/cache_metrics.h"

namespace abel {
namespace metrics {

class serializer {
  public:

    virtual ~serializer() = default;

    std::string format(const std::vector<cache_metrics> &) const;

    virtual void format(std::ostream &stdout, const std::vector<cache_metrics> &) const = 0;
};
}  // namespace metrics
}  // namespace abel


#endif  // ABEL_ABEL_METRICS_SERIALIZER_H_
