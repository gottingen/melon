// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include "abel/metrics/bucket.h"
#include "abel/base/profile.h"

namespace abel {
namespace metrics {

bucket bucket_builder::liner_values(double start, double width, size_t number) {
    ABEL_ASSERT(width > 0);
    std::vector<double> ret;
    ret.reserve(number);
    double value = start;
    for (size_t i = 0; i < number; ++i) {
        ret.push_back(value);
        value += width;
    }
    return ret;
}

bucket bucket_builder::exponential_values(double start, double factor, size_t number) {
    ABEL_ASSERT(factor > 0);
    ABEL_ASSERT(start > 0);
    std::vector<double> ret;
    ret.reserve(number);
    double value = start;
    for (size_t i = 0; i < number; ++i) {
        ret.push_back(value);
        value *= factor;
    }
    return ret;
}

bucket bucket_builder::liner_duration(abel::duration start, abel::duration width, size_t num) {
    abel::duration value = start;
    ABEL_ASSERT(width.to_double_microseconds() > 0);
    std::vector<double> ret;
    ret.reserve(num);
    for (size_t i = 0; i < num; ++i) {
        ret.push_back(value.to_double_microseconds());
        value += width;
    }
    return ret;
}

bucket bucket_builder::exponential_duration(abel::duration start, uint64_t factor, size_t num) {
    abel::duration value = start;
    ABEL_ASSERT(factor > 1);
    std::vector<double> ret;
    ret.reserve(num);
    for (size_t i = 0; i < num; ++i) {
        ret.push_back(value.to_double_microseconds());
        value *= factor;
    }
    return ret;
}

}  // namespace metrics
}  // namespace abel
