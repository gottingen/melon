//
// Created by liyinbin on 2020/2/8.
//

#include <abel/metrics/gauge.h>
#include <abel/metrics/metrics_type.h>

namespace abel {
namespace metrics {

gauge::gauge(const double value) : _value{value} {}

void gauge::inc() noexcept
{
    inc(1.0);
}

void gauge::inc(const double value) noexcept
{
    if (value < 0.0) {
        return;
    }
    change(value);
}

void gauge::dec() noexcept
{
    dec(1.0);
}

void gauge::dec(const double value) noexcept
{
    if (value < 0.0) {
        return;
    }
    change(-1.0 * value);
}

void gauge::update(const double value) noexcept
{
    _value.store(value);
}

void gauge::change(const double value) noexcept
{
    auto current = _value.load();
    while (!_value.compare_exchange_weak(current, current + value))
        ;
}


double gauge::value() const noexcept
{
    return _value;
}

cache_metrics gauge::collect() const noexcept
{
    cache_metrics cm;
    cm.gauge.value = value();
    cm.type = metrics_type::mt_gauge;
    return cm;
}

} //namespace metrics
} //namespace abel