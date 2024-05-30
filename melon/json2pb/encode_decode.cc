//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#include <string>
#include <sstream>
#include <melon/json2pb/encode_decode.h>

namespace json2pb {

    inline int match_pattern(const std::string &str, int index) {
        //pattern: _Zxxx_
        const char digit = '0';
        const int pattern_length = 6;

        int length = str.size();
        if (length <= index || length - index < pattern_length) {
            return -1;
        }
        if (str[index] != '_' ||
            str[index + 1] != 'Z' ||
            str[index + 5] != '_' ||
            !isdigit(str[index + 2]) ||
            !isdigit(str[index + 3]) ||
            !isdigit(str[index + 4])) {
            return -1;
        }
        int sum = (str[index + 2] - digit) * 100 + (str[index + 3] - digit) * 10 + (str[index + 4] - digit);
        return sum < 256 ? sum : -1;
    }

    bool encode_name(const std::string &content, std::string &encoded_content) {
        int index = 0;
        size_t begin = 0;
        bool convert = false;
        for (std::string::const_iterator it = content.begin(); it != content.end(); ++it, ++index) {
            if ((!isalnum(*it) &&
                 (*it != '_')) ||
                (it == content.begin() &&
                 isdigit(*it))) {
                if (!convert) {
                    encoded_content.clear();
                    encoded_content.reserve(2 * content.size());
                    convert = true;
                }
                encoded_content.append(content, begin, index - begin);
                begin = index + 1;

                char pattern[6];
                pattern[0] = '_';
                pattern[1] = 'Z';
                int first = *it / 100;
                int second = (*it - first * 100) / 10;
                int third = *it - first * 100 - second * 10;
                pattern[2] = first + '0';
                pattern[3] = second + '0';
                pattern[4] = third + '0';
                pattern[5] = '_';
                encoded_content.append(pattern, sizeof(pattern));
            }
        }
        if (!convert) {
            return false;
        } else {
            encoded_content.append(content, begin, index - begin);
            return true;
        }
    }

    bool decode_name(const std::string &content, std::string &decoded_content) {
        const int pattern_length = 6;
        int begin = 0;
        int second = 0;
        bool convert = false;
        for (std::string::const_iterator it = content.begin(); it < content.end(); ++it, ++second) {
            if (*it != '_') {
                continue;
            }
            int val = match_pattern(content, second);
            if (val != -1) {
                if (!convert) {
                    decoded_content.clear();
                    decoded_content.reserve(content.size());
                    convert = true;
                }
                decoded_content.append(content, begin, second - begin);
                decoded_content.push_back(static_cast<char>(val));
                second += pattern_length - 1;
                begin = second + 1;
                it += pattern_length - 1;
            }
        }
        if (!convert) {
            return false;
        } else {
            decoded_content.append(content, begin, second - begin);
            return true;
        }
    }

} // namespace json2pb
