
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "melon/metrics/bucket.h"
#include "melon/base/profile.h"
#include "melon/log/logging.h"

namespace melon {

    bucket bucket_builder::liner_values(double start, double width, size_t number) {
        MELON_CHECK(width > 0);
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
        MELON_CHECK(factor > 0);
        MELON_CHECK(start > 0);
        std::vector<double> ret;
        ret.reserve(number);
        double value = start;
        for (size_t i = 0; i < number; ++i) {
            ret.push_back(value);
            value *= factor;
        }
        return ret;
    }

    bucket bucket_builder::liner_duration(melon::duration start, melon::duration width, size_t num) {
        melon::duration value = start;
        MELON_CHECK(width.to_double_microseconds() > 0);
        std::vector<double> ret;
        ret.reserve(num);
        for (size_t i = 0; i < num; ++i) {
            ret.push_back(value.to_double_microseconds());
            value += width;
        }
        return ret;
    }

    bucket bucket_builder::exponential_duration(melon::duration start, uint64_t factor, size_t num) {
        melon::duration value = start;
        MELON_CHECK(factor > 1);
        std::vector<double> ret;
        ret.reserve(num);
        for (size_t i = 0; i < num; ++i) {
            ret.push_back(value.to_double_microseconds());
            value *= factor;
        }
        return ret;
    }

}  // namespace abel
