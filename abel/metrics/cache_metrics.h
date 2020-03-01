//
// Created by liyinbin on 2020/2/7.
//

#ifndef ABEL_METRICS_CACHE_METRICS_H_
#define ABEL_METRICS_CACHE_METRICS_H_

#include <abel/metrics/metrics_type.h>
#include <abel/metrics/scope_family.h>
#include <vector>
#include <unordered_map>
#include <string>

namespace abel {
    namespace metrics {


        struct cache_metrics {

            metrics_type type{metrics_type::mt_untyped};

            std::string name;
            scope_family_ptr family;
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

            std::int64_t timestamp_ms = 0;
        };

    } //namespace metrics
} //namespace abel

#endif //ABEL_METRICS_CACHE_METRICS_H_
