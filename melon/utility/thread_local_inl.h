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


// Date: Tue Sep 16 12:39:12 CST 2014

#ifndef MUTIL_THREAD_LOCAL_INL_H
#define MUTIL_THREAD_LOCAL_INL_H

namespace mutil {

namespace detail {

template <typename T>
class ThreadLocalHelper {
public:
    inline static T* get() {
        if (__builtin_expect(value != NULL, 1)) {
            return value;
        }
        value = new (std::nothrow) T;
        if (value != NULL) {
            mutil::thread_atexit(delete_object<T>, value);
        }
        return value;
    }
    static MELON_THREAD_LOCAL T* value;
};

template <typename T> MELON_THREAD_LOCAL T* ThreadLocalHelper<T>::value = NULL;

}  // namespace detail

template <typename T> inline T* get_thread_local() {
    return detail::ThreadLocalHelper<T>::get();
}

}  // namespace mutil

#endif  // MUTIL_THREAD_LOCAL_INL_H
