//
// Created by liyinbin on 2020/2/7.
//

#ifndef ABEL_METRICS_SCOPE_FAMILY_H_
#define ABEL_METRICS_SCOPE_FAMILY_H_

#include <unordered_map>
#include <string>
#include <memory>

namespace abel {
    namespace metrics {

//todo: use string_view ?

        struct scope_family {
            std::string prefix;
            std::string separator;
            std::unordered_map<std::string, std::string> tags;

            scope_family() = default;

            scope_family(const std::string &p, const std::string &s,
                         const std::unordered_map<std::string, std::string> &t)
                    : prefix(p), separator(s), tags(t) {

            }
        };

        typedef std::shared_ptr<scope_family> scope_family_ptr;

    } //namespace metrics
} //namespace abel

#endif //ABEL_METRICS_SCOPE_FAMILY_H_
