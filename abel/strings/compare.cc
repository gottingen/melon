// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/strings/compare.h"
#include "abel/strings/ascii.h"

namespace abel {

int compare_case(std::string_view a, std::string_view b) {
    std::string_view::const_iterator ai = a.begin();
    std::string_view::const_iterator bi = b.begin();

    while (ai != a.end() && bi != b.end()) {
        int ca = ascii::to_lower(*ai++);
        int cb = ascii::to_lower(*bi++);

        if (ca == cb) {
            continue;
        }
        if (ca < cb) {
            return -1;
        } else {
            return +1;
        }
    }

    if (ai == a.end() && bi != b.end()) {
        return +1;
    } else if (ai != a.end() && bi == b.end()) {
        return -1;
    } else {
        return 0;
    }
}

}  // namespace abel
