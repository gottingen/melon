//
// Created by liyinbin on 2019/12/8.
//

#include <abel/strings/trim.h>
#include <algorithm>
#include <cstring>

namespace abel {

std::string &trim_all (std::string *str) {
    trim_left(str);
    trim_right(str);
    return *str;
}

std::string &trim_all (std::string *str, const char *drop) {
    std::string::size_type pos = str->find_last_not_of(drop);
    if (pos != std::string::npos) {
        str->erase(pos + 1);
        pos = str->find_first_not_of(drop);
        if (pos != std::string::npos)
            str->erase(0, pos);
    } else {
        str->erase(str->begin(), str->end());
    }

    return *str;
}

std::string &trim_all (std::string *str, const std::string &drop) {
    std::string::size_type pos = str->find_last_not_of(drop);
    if (pos != std::string::npos) {
        str->erase(pos + 1);
        pos = str->find_first_not_of(drop);
        if (pos != std::string::npos)
            str->erase(0, pos);
    } else
        str->erase(str->begin(), str->end());

    return *str;
}

std::string trim_all (const std::string &str, const char *drop) {
    // trim beginning
    std::string::size_type pos1 = str.find_first_not_of(drop);
    if (pos1 == std::string::npos)
        return std::string();

    // copy middle and end
    std::string out = str.substr(pos1, std::string::npos);

    // trim end
    std::string::size_type pos2 = out.find_last_not_of(drop);
    if (pos2 != std::string::npos)
        out.erase(pos2 + 1);

    return out;
}

std::string trim_all (const std::string &str,
                      const std::string &drop) {
    std::string out;
    out.reserve(str.size());

    // trim beginning
    std::string::const_iterator it = str.begin();
    while (it != str.end() && drop.find(*it) != std::string::npos)
        ++it;

    // copy middle and end
    out.resize(str.end() - it);
    std::copy(it, str.end(), out.begin());

    // trim end
    std::string::size_type pos = out.find_last_not_of(drop);
    if (pos != std::string::npos)
        out.erase(pos + 1);

    return out;
}

/******************************************************************************/

std::string &trim_right (std::string *str) {
    auto it = std::find_if_not(str->rbegin(), str->rend(), abel::ascii::is_space);
    str->erase(str->rend() - it);
    return *str;
}

std::string &trim_right (std::string *str, const char *drop) {
    str->erase(str->find_last_not_of(drop) + 1, std::string::npos);
    return *str;
}

std::string &trim_right (std::string *str, const std::string &drop) {
    str->erase(str->find_last_not_of(drop) + 1, std::string::npos);
    return *str;
}

std::string trim_right (const std::string &str, const char *drop) {
    std::string::size_type pos = str.find_last_not_of(drop);
    if (pos == std::string::npos)
        return std::string();

    return str.substr(0, pos + 1);
}

std::string trim_right (const std::string &str, const std::string &drop) {
    std::string::size_type pos = str.find_last_not_of(drop);
    if (pos == std::string::npos)
        return std::string();

    return str.substr(0, pos + 1);
}

/******************************************************************************/

std::string &trim_left (std::string *str) {
    auto it = std::find_if_not(str->begin(), str->end(), abel::ascii::is_space);
    str->erase(str->begin(), it);
    return *str;
}

std::string &trim_left (std::string *str, const char *drop) {
    str->erase(0, str->find_first_not_of(drop));
    return *str;
}

std::string &trim_left (std::string *str, const std::string &drop) {
    str->erase(0, str->find_first_not_of(drop));
    return *str;
}

std::string trim_left (const std::string &str, const char *drop) {
    std::string::size_type pos = str.find_first_not_of(drop);
    if (pos == std::string::npos)
        return std::string();

    return str.substr(pos, std::string::npos);
}

std::string trim_left (const std::string &str, const std::string &drop) {
    std::string::size_type pos = str.find_first_not_of(drop);
    if (pos == std::string::npos)
        return std::string();

    return str.substr(pos, std::string::npos);
}

void trim_complete (std::string *str) {
    auto stripped = trim_all(*str);

    if (stripped.empty()) {
        str->clear();
        return;
    }

    auto input_it = stripped.begin();
    auto input_end = stripped.end();
    auto output_it = &(*str)[0];
    bool is_ws = false;

    for (; input_it < input_end; ++input_it) {
        if (is_ws) {
            // Consecutive whitespace?  Keep only the last.
            is_ws = abel::ascii::is_space(*input_it);
            if (is_ws)
                --output_it;
        } else {
            is_ws = abel::ascii::is_space(*input_it);
        }

        *output_it = *input_it;
        ++output_it;
    }

    str->erase(output_it - &(*str)[0]);
}

} //namespace able