//
// Created by liyinbin on 2020/2/8.
//

#ifndef ABEL_METRICS_GAUGE_H_
#define ABEL_METRICS_GAUGE_H_

#include <abel/metrics/cache_metrics.h>
#include <atomic>
#include <memory>

namespace abel {
    namespace metrics {

        class gauge {
        public:
            gauge() = default;

            explicit gauge(const double value);

            void update(const double value) noexcept;

            void inc() noexcept;

            void inc(const double v) noexcept;

            void dec() noexcept;

            void dec(const double v) noexcept;

            double value() const noexcept;

            cache_metrics collect() const noexcept;

        private:
            void change(double value) noexcept;

        private:
            std::atomic<double> _value{0.0};
        };

        typedef std::shared_ptr<gauge> gauge_ptr;

    } //namespace metrics
} //namespace abel

#endif //ABEL_METRICS_GAUGE_H_
