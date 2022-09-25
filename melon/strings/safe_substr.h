
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_STRINGS_SAFE_SUBSTR_H_
#define MELON_STRINGS_SAFE_SUBSTR_H_

namespace melon {

    inline std::string_view safe_substr(const std::string_view &sv, size_t pos = 0, size_t n = std::string_view::npos) noexcept {
        if (pos > sv.size()) {
            pos = sv.size();
        }
        if (n > (sv.size() - pos)) {
            n = sv.size() - pos;
        }
        return std::string_view(sv.data() + pos, n);
    }

}  // namespace melon

#endif // MELON_STRINGS_SAFE_SUBSTR_H_

