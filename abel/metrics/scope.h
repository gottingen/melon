//
// Created by liyinbin on 2020/2/8.
//

#ifndef ABEL_METRICS_SCOPE_H_
#define ABEL_METRICS_SCOPE_H_


#include <abel/metrics/counter.h>
#include <abel/metrics/gauge.h>
#include <abel/metrics/histogram.h>
#include <abel/metrics/cache_metrics.h>
#include <abel/metrics/timer.h>
#include <memory>
#include <mutex>
#include <string>

namespace abel {
namespace metrics {

// forward declare
class scope;
typedef std::shared_ptr<scope> scope_ptr;

class scope {
public:

    static  std::shared_ptr<scope> new_root_scope(const std::string &prefix, const std::string & separator,
                                                const std::unordered_map<std::string, std::string> &tags);

    std::shared_ptr<counter> get_counter(const std::string &prefix);

    std::shared_ptr<gauge> get_gauge(const std::string &prefix);

    std::shared_ptr<histogram> get_histogram(const std::string &prefix, const bucket &bucket);

    std::shared_ptr<timer>     get_timer(const std::string &prefix, const bucket &bucket);

    std::shared_ptr<scope> sub_scope(const std::string &prefix);

    std::shared_ptr<scope> tagged(const std::unordered_map<std::string, std::string> &tags);

    std::unordered_map<std::string, std::string> tags() const;

    std::string separator() const;

    std::string prefix() const;

    void collect(std::vector<cache_metrics> &res);

private:
    scope(const scope_family_ptr &family);

    std::string fully_qualified_name(const std::string &name);

    std::string scope_id(
        const std::string &prefix,
        const std::unordered_map<std::string, std::string> &tags);

    std::shared_ptr<scope> sub_scope(
        const std::string &prefix,
        const std::unordered_map<std::string, std::string> &tags);


private:
    scope_family_ptr                                     _family;
    std::unordered_map<std::string, counter_ptr>        _counters;
    std::unordered_map<std::string, gauge_ptr>          _gauges;
    std::unordered_map<std::string, histogram_ptr>      _histograms;
    std::unordered_map<std::string, scope_ptr>          _scopes;
    std::unordered_map<std::string, timer_ptr>          _timers;

    std::mutex                                         _counter_mutex;
    std::mutex                                         _gauge_mutex;
    std::mutex                                         _histogram_mutex;
    std::mutex                                         _scope_mutex;
    std::mutex                                         _timer_mutex;
};


} //namespace metrics
} //namespace abel

#endif //ABEL_METRICS_SCOPE_H_
