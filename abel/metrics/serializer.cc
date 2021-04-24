// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include "abel/metrics/serializer.h"
#include <sstream>

namespace abel {
namespace metrics {

std::string serializer::format(const std::vector<cache_metrics> &metrics) const {
    std::ostringstream ss;
    format(ss, metrics);
    return ss.str();
}
}  // namespace metrics
} //namespace
