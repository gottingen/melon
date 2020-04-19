//
// Created by liyinbin on 2019/12/8.
//

#include <abel/strings/trim.h>
#include <algorithm>
#include <cstring>

namespace abel {

    std::string &trim_all(std::string *str) {
        trim_left(str);
        trim_right(str);
        return *str;
    }

    std::string &trim_all(std::string *str, abel::string_view drop) {
        std::string::size_type pos = str->find_last_not_of(drop.data(), drop.size());
        if (pos != std::string::npos) {
            str->erase(pos + 1);
            pos = str->find_first_not_of(drop.data(), drop.size());
            if (pos != std::string::npos)
                str->erase(0, pos);
        } else {
            str->erase(str->begin(), str->end());
        }

        return *str;
    }

    abel::string_view trim_all(abel::string_view str, abel::string_view drop) {
        // trim beginning
        abel::string_view::size_type pos1 = str.find_first_not_of(drop);
        if (pos1 == abel::string_view::npos)
            return abel::string_view();

        // copy middle and end
        abel::string_view out = str.substr(pos1, std::string::npos);

        // trim end
        std::string::size_type pos2 = out.find_last_not_of(drop);
        if (pos2 != abel::string_view::npos)
            out = out.substr(0, pos2);

        return out;
    }

/******************************************************************************/

    std::string &trim_right(std::string *str) {
        auto it = std::find_if_not(str->rbegin(), str->rend(), abel::ascii::is_space);
        str->erase(str->rend() - it);
        return *str;
    }

    std::string &trim_right(std::string *str, abel::string_view drop) {
        str->erase(str->find_last_not_of(drop.data(), drop.size()) + 1, std::string::npos);
        return *str;
    }

    abel::string_view trim_right(abel::string_view str, abel::string_view drop) {
        abel::string_view::size_type pos = str.find_last_not_of(drop);
        if (pos == abel::string_view::npos)
            return abel::string_view();

        return str.substr(0, pos + 1);
    }

/******************************************************************************/

    std::string &trim_left(std::string *str) {
        auto it = std::find_if_not(str->begin(), str->end(), abel::ascii::is_space);
        str->erase(str->begin(), it);
        return *str;
    }

    std::string &trim_left(std::string *str, abel::string_view drop) {
        str->erase(0, str->find_first_not_of(drop.data(), drop.size()));
        return *str;
    }

    abel::string_view trim_left(abel::string_view str, abel::string_view drop) {
        abel::string_view::size_type pos = str.find_first_not_of(drop);
        if (pos == abel::string_view::npos)
            return abel::string_view();

        return str.substr(pos, std::string::npos);
    }

    void trim_complete(std::string *str) {
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
