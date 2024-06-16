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


// Date: Sun Dec  4 14:57:27 CST 2016

#ifndef MUTIL_CASE_IGNORED_FLAT_MAP_H
#define MUTIL_CASE_IGNORED_FLAT_MAP_H

#include <melon/base/containers/flat_map.h>

namespace mutil {

// Using ascii_tolower instead of ::tolower shortens 150ns in
// FlatMapTest.perf_small_string_map (with -O2 added, -O0 by default)
// note: using char caused crashes on ubuntu 20.04 aarch64 (VM on apple M1)
inline char ascii_tolower(int/*note*/ c) {
    extern const signed char* const g_tolower_map;
    return g_tolower_map[c];
}

struct CaseIgnoredHasher {
    size_t operator()(const std::string& s) const {
        std::size_t result = 0;                                               
        for (std::string::const_iterator i = s.begin(); i != s.end(); ++i) {
            result = result * 101 + ascii_tolower(*i);
        }
        return result;
    }
    size_t operator()(const char* s) const {
        std::size_t result = 0;                                               
        for (; *s; ++s) {
            result = result * 101 + ascii_tolower(*s);
        }
        return result;
    }
};

struct CaseIgnoredEqual {
    // NOTE: No overload for mutil::StringPiece. It needs strncasecmp
    // which is much slower than strcasecmp in micro-benchmarking. As a
    // result, methods in HttpHeader does not accept StringPiece as well.
    bool operator()(const std::string& s1, const std::string& s2) const {
        return s1.size() == s2.size() &&
            strcasecmp(s1.c_str(), s2.c_str()) == 0;
    }
    bool operator()(const std::string& s1, const char* s2) const
    { return strcasecmp(s1.c_str(), s2) == 0; }
};

template <typename T>
class CaseIgnoredFlatMap : public mutil::FlatMap<
    std::string, T, CaseIgnoredHasher, CaseIgnoredEqual> {};

class CaseIgnoredFlatSet : public mutil::FlatSet<
    std::string, CaseIgnoredHasher, CaseIgnoredEqual> {};

} // namespace mutil

#endif  // MUTIL_CASE_IGNORED_FLAT_MAP_H
