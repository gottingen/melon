//
// Created by liyinbin on 2020/2/7.
//

#ifndef ABEL_METRICS_METRICS_TYPE_H_
#define ABEL_METRICS_METRICS_TYPE_H_

namespace abel {
namespace metrics {

enum class metrics_type {
    mt_untyped,
    mt_counter,
    mt_timer,
    mt_gauge,
    mt_histogram
};

} //namespace metrics
} //namespace abel

#endif //ABEL_METRICS_METRICS_TYPE_H_
