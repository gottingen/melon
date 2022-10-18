
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_METRICS_CACHE_METRIC_H_
#define MELON_METRICS_CACHE_METRIC_H_

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

namespace melon {

    enum class metrics_type {
        mt_untyped,
        mt_counter,
        mt_timer,
        mt_gauge,
        mt_histogram
    };


    struct cache_metrics {

        metrics_type type{metrics_type::mt_untyped};

        std::string name;
        std::string help;
        std::unordered_map<std::string, std::string> tags;

        struct cached_counter {
            double value = 0.0;
        };
        cached_counter counter;

        struct cached_gauge {
            double value = 0.0;
        };

        cached_gauge gauge;

        struct cached_bucket {
            std::uint64_t cumulative_count = 0;
            double upper_bound = 0.0;
        };

        struct cached_histogram {
            std::uint64_t sample_count = 0;
            double sample_sum = 0.0;
            std::vector<cached_bucket> bucket;
        };
        cached_histogram histogram;

    };

}  // namespace melon

#endif  // MELON_METRICS_CACHE_METRIC_H_
