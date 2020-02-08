//
// Created by liyinbin on 2020/2/8.
//

#ifndef ABEL_ABEL_METRICS_SERIALIZER_H_
#define ABEL_ABEL_METRICS_SERIALIZER_H_

#include <abel/metrics/cache_metrics.h>
#include <iosfwd>
#include <string>
#include <vector>

namespace abel {
namespace metrics {

class serializer {
public:

    virtual ~serializer () = default;

    virtual std::string format (const std::vector<cache_metrics> &) const;

    virtual void format (std::ostream &stdout, const std::vector<cache_metrics> &) const = 0;
};
} //namespace metrics
} //namespace abel


#endif //ABEL_ABEL_METRICS_SERIALIZER_H_
