//
// Created by liyinbin on 2020/2/8.
//

#ifndef ABEL_METRICS_PROM_SERIALIZER_H_
#define ABEL_METRICS_PROM_SERIALIZER_H_

#include <abel/metrics/cache_metrics.h>
#include <abel/metrics/serializer.h>
#include <iosfwd>
#include <string>
#include <vector>

namespace abel {
    namespace metrics {

        class prometheus_serializer : public serializer {
        public:
            using serializer::serializer;

            void format(std::ostream &stdout, const std::vector<cache_metrics> &) const override;
        };

    } //namespace metrics
} //namespace abel

#endif //ABEL_METRICS_PROM_SERIALIZER_H_
