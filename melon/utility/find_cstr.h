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


// Date: Tue Jun 23 15:03:24 CST 2015

#ifndef MUTIL_FIND_CSTR_H
#define MUTIL_FIND_CSTR_H

#include <string>
#include <map>
#include <algorithm>
#include <melon/base/thread_local.h>

// Find c-string in maps with std::string as keys without memory allocations.
// Example:
//   std::map<std::string, int> string_map;
//
//   string_map.find("hello");         // constructed a temporary std::string
//                                     // which needs memory allocation.
//
//   find_cstr(string_map, "hello");   // no allocation.
//
// You can specialize find_cstr for other maps.
// Possible prototypes are:
//   const_iterator find_cstr(const Map& map, const char* key);
//   iterator       find_cstr(Map& map, const char* key);
//   const_iterator find_cstr(const Map& map, const char* key, size_t length);
//   iterator       find_cstr(Map& map, const char* key, size_t length);

namespace mutil {

struct StringMapThreadLocalTemp {
    bool initialized;
    char buf[sizeof(std::string)];

    static void delete_tls(void* buf) {
        StringMapThreadLocalTemp* temp = (StringMapThreadLocalTemp*)buf;
        if (temp->initialized) {
            temp->initialized = false;
            std::string* temp_string = (std::string*)temp->buf;
            temp_string->~basic_string();
        }
    }

    inline std::string* get_string(const char* key) {
        if (!initialized) {
            initialized = true;
            std::string* tmp = new (buf) std::string(key);
            thread_atexit(delete_tls, this);
            return tmp;
        } else {
            std::string* tmp = (std::string*)buf;
            tmp->assign(key);
            return tmp;
        }
    }

    inline std::string* get_string(const char* key, size_t length) {
        if (!initialized) {
            initialized = true;
            std::string* tmp = new (buf) std::string(key, length);
            thread_atexit(delete_tls, this);
            return tmp;
        } else {
            std::string* tmp = (std::string*)buf;
            tmp->assign(key, length);
            return tmp;
        }
    }

    inline std::string* get_lowered_string(const char* key) {
        std::string* tmp = get_string(key);
        std::transform(tmp->begin(), tmp->end(), tmp->begin(), ::tolower);
        return tmp;
    }
    
    inline std::string* get_lowered_string(const char* key, size_t length) {
        std::string* tmp = get_string(key, length);
        std::transform(tmp->begin(), tmp->end(), tmp->begin(), ::tolower);
        return tmp;
    }
};

extern thread_local StringMapThreadLocalTemp tls_stringmap_temp;

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::const_iterator
find_cstr(const std::map<std::string, T, C, A>& m, const char* key) {
    return m.find(*tls_stringmap_temp.get_string(key));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::iterator
find_cstr(std::map<std::string, T, C, A>& m, const char* key) {
    return m.find(*tls_stringmap_temp.get_string(key));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::const_iterator
find_cstr(const std::map<std::string, T, C, A>& m,
          const char* key, size_t length) {
    return m.find(*tls_stringmap_temp.get_string(key, length));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::iterator
find_cstr(std::map<std::string, T, C, A>& m,
          const char* key, size_t length) {
    return m.find(*tls_stringmap_temp.get_string(key, length));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::const_iterator
find_lowered_cstr(const std::map<std::string, T, C, A>& m, const char* key) {
    return m.find(*tls_stringmap_temp.get_lowered_string(key));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::iterator
find_lowered_cstr(std::map<std::string, T, C, A>& m, const char* key) {
    return m.find(*tls_stringmap_temp.get_lowered_string(key));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::const_iterator
find_lowered_cstr(const std::map<std::string, T, C, A>& m,
                  const char* key, size_t length) {
    return m.find(*tls_stringmap_temp.get_lowered_string(key, length));
}

template <typename T, typename C, typename A>
typename std::map<std::string, T, C, A>::iterator
find_lowered_cstr(std::map<std::string, T, C, A>& m,
                  const char* key, size_t length) {
    return m.find(*tls_stringmap_temp.get_lowered_string(key, length));
}

}  // namespace mutil

#endif  // MUTIL_FIND_CSTR_H
