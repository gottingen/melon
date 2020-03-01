//
// Created by liyinbin on 2020/2/8.
//

#ifndef ABEL_ABEL_METRICS_STOP_WATCHER_H_
#define ABEL_ABEL_METRICS_STOP_WATCHER_H_

#include <chrono>
#include <memory>
#include <abel/chrono/time.h>

namespace abel {
    namespace metrics {
        class timer;

        class stop_watcher {
        public:
            stop_watcher(abel::abel_time, std::shared_ptr<timer> recorder);

            void stop();

        private:
            const abel::abel_time _start;
            std::shared_ptr<timer> _recorder;
        };
    } //namespace metrics
} //namespace abel


#endif //ABEL_ABEL_METRICS_STOP_WATCHER_H_
