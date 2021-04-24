// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_METRICS_BUCKET_H_
#define ABEL_METRICS_BUCKET_H_

#include <vector>
#include "abel/chrono/time.h"

namespace abel {
namespace metrics {
typedef std::vector<double> bucket;

struct bucket_builder {

    static bucket liner_values(double start, double width, size_t number);

    static bucket exponential_values(double start, double factor, size_t num);

    static bucket liner_duration(abel::duration start, abel::duration width, size_t num);

    static bucket exponential_duration(abel::duration start, uint64_t factor, size_t num);
};

}  // namespace metrics
}  // namespace abel

#endif  // ABEL_METRICS_BUCKET_H_
