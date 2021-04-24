// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_ABEL_METRICS_STOP_WATCHER_H_
#define ABEL_ABEL_METRICS_STOP_WATCHER_H_

#include <chrono>
#include <memory>
#include "abel/chrono/time.h"

namespace abel {
namespace metrics {
class timer;

class stop_watcher {
  public:
    stop_watcher(abel::time_point, std::shared_ptr<timer> recorder);

    void stop();

  private:
    const abel::time_point _start;
    std::shared_ptr<timer> _recorder;
};
}  // namespace metrics
}  // namespace abel


#endif  // ABEL_ABEL_METRICS_STOP_WATCHER_H_
