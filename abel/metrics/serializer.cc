//
// Created by liyinbin on 2020/2/8.
//

#include <abel/metrics/serializer.h>
#include <sstream>

namespace abel {
    namespace metrics {

        std::string serializer::format(const std::vector<cache_metrics> &metrics) const {
            std::ostringstream ss;
            format(ss, metrics);
            return ss.str();
        }
    } //namespace metrics
} //namespace
