// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_CHRONO_STOP_WATCHER_H_
#define ABEL_CHRONO_STOP_WATCHER_H_

#include "abel/chrono/clock.h"

namespace abel {

struct stop_watcher_tag {
};

class stop_watcher {
  public:
    stop_watcher() = default;

    ~stop_watcher() = default;

    explicit stop_watcher(stop_watcher_tag) {
        start();
    }

    void start() {
        _start = abel::time_now();
        _stop = _start;
    }

    void stop() {
        _stop = abel::time_now();
    }

    abel::duration elapsed() {
        return _stop - _start;
    }

  private:
    abel::time_point _start;
    abel::time_point _stop;
};
}  // namespace abel

#endif  // ABEL_CHRONO_STOP_WATCHER_H_
