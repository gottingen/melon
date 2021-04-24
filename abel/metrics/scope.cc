// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include <sstream>
#include <algorithm>

#include "abel/metrics/scope.h"
#include "abel/metrics/scope_family.h"

namespace abel {
namespace metrics {

scope::scope(const scope_family_ptr &family)
        : _family(family) {

}

std::shared_ptr<scope> scope::new_root_scope(const std::string &prefix, const std::string &separator,
                                             const std::unordered_map<std::string, std::string> &tags) {
    scope_family_ptr sf(new scope_family(prefix, separator, tags));
    std::shared_ptr<scope> s(new scope(sf));
    return s;
}

std::shared_ptr<counter> scope::get_counter(const std::string &prefix) {
    std::unique_lock<std::mutex> lk(_counter_mutex);
    auto it = _counters.find(prefix);
    if (it != _counters.end()) {
        return it->second;
    }

    counter_ptr counterPtr(new counter());
    _counters[prefix] = counterPtr;
    return counterPtr;
}

std::shared_ptr<gauge> scope::get_gauge(const std::string &prefix) {
    std::unique_lock<std::mutex> lk(_gauge_mutex);
    auto it = _gauges.find(prefix);
    if (it != _gauges.end()) {
        return it->second;
    }

    gauge_ptr gaugePtr(new gauge());
    _gauges[prefix] = gaugePtr;
    return gaugePtr;
}

std::shared_ptr<histogram> scope::get_histogram(const std::string &prefix, const bucket &bucket) {
    std::unique_lock<std::mutex> lk(_histogram_mutex);
    auto it = _histograms.find(prefix);
    if (it != _histograms.end()) {
        return it->second;
    }

    histogram_ptr histogramPtr(new histogram(bucket));
    _histograms[prefix] = histogramPtr;
    return histogramPtr;
}

std::shared_ptr<timer> scope::get_timer(const std::string &prefix, const bucket &bucket) {
    std::unique_lock<std::mutex> lk(_timer_mutex);
    auto it = _timers.find(prefix);
    if (it != _timers.end()) {
        return it->second;
    }

    timer_ptr timerPtr = timer::new_timer(bucket);
    _timers[prefix] = timerPtr;
    return timerPtr;
}

std::shared_ptr<scope> scope::sub_scope(const std::string &prefix) {
    return sub_scope(fully_qualified_name(prefix),
                     std::unordered_map<std::string, std::string>{});
}

std::shared_ptr<scope> scope::tagged(const std::unordered_map<std::string, std::string> &tags) {
    return sub_scope(_family->prefix, tags);
}

std::unordered_map<std::string, std::string> scope::tags() const {
    return _family->tags;
}

std::string scope::separator() const {
    return _family->separator;
}

std::string scope::prefix() const {
    return _family->prefix;
}

void scope::collect(std::vector<cache_metrics> &res) {

    {
        std::unique_lock<std::mutex> lk(_counter_mutex);
        for (auto const &counter : _counters) {
            auto cm = counter.second->collect();
            cm.name = counter.first;
            cm.family = _family;
            res.push_back(cm);
        }
    }
    {
        std::unique_lock<std::mutex> lk(_gauge_mutex);
        for (auto const &gauge : _gauges) {
            auto gm = gauge.second->collect();
            gm.name = gauge.first;
            gm.family = _family;
            res.push_back(gm);
        }
    }
    {
        std::unique_lock<std::mutex> lk(_histogram_mutex);
        for (auto const &histogram : _histograms) {
            auto hm = histogram.second->collect();
            hm.name = histogram.first;
            hm.family = _family;
            res.push_back(hm);
        }
    }
    {
        std::unique_lock<std::mutex> lk(_timer_mutex);
        for (auto const &timer : _timers) {
            auto tm = timer.second->collect();
            tm.name = timer.first;
            tm.family = _family;
            res.push_back(tm);
        }

    }

    {
        std::unique_lock<std::mutex> lk(_scope_mutex);
        for (auto const &scope : _scopes) {
            scope.second->collect(res);
        }
    }
}

std::string scope::fully_qualified_name(const std::string &name) {
    if (_family->prefix == "") {
        return name;
    }

    std::string str;
    str.reserve(_family->prefix.length() + _family->separator.length() + name.length());
    std::ostringstream stream(str);
    stream << _family->prefix << _family->separator << name;

    return stream.str();
}

std::string scope::scope_id(
        const std::string &prefix,
        const std::unordered_map<std::string, std::string> &tags) {
    std::vector<std::string> keys;
    keys.reserve(tags.size());
    std::transform(
            std::begin(tags), std::end(tags), std::back_inserter(keys),
            [](const std::pair<std::string, std::string> &tag) { return tag.first; });

    std::sort(keys.begin(), keys.end());

    std::string str;
    str.reserve(prefix.length() + keys.size() * 20);
    std::ostringstream stream(str);

    stream << prefix;
    stream << "+";

    for (auto it = keys.begin(); it < keys.end(); it++) {
        auto key = *it;
        stream << key << "=" << tags.at(key);

        if (it != keys.end() - 1) {
            stream << ",";
        }
    }

    return stream.str();
}

std::shared_ptr<scope> scope::sub_scope(
        const std::string &prefix,
        const std::unordered_map<std::string, std::string> &tags) {
    std::unordered_map<std::string, std::string> new_tags;

    // Insert the new tags second as they take priority over the scope's tags.
    for (auto const &tag : _family->tags) {
        new_tags.insert(tag);
    }

    for (auto const &tag : tags) {
        new_tags.insert(tag);
    }

    auto id = scope_id(prefix, new_tags);

    std::lock_guard<std::mutex> lock(_scope_mutex);
    auto it = _scopes.find(id);
    if (it != _scopes.end()) {
        return it->second;
    }

    scope_family_ptr sf(new scope_family(prefix, _family->separator, new_tags));
    std::shared_ptr<scope> s(new scope(sf));

    _scopes[id] = s;
    return s;
}

}  // namespace metrics
}  // namespace abel

