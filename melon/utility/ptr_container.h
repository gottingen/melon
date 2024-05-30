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


// Date: Fri Sep  7 12:15:23 CST 2018

#ifndef MUTIL_PTR_CONTAINER_H
#define MUTIL_PTR_CONTAINER_H

namespace mutil {

// Manage lifetime of a pointer. The key difference between PtrContainer and
// unique_ptr is that PtrContainer can be copied and the pointer inside is
// deeply copied or constructed on-demand.
template <typename T>
class PtrContainer {
public:
    PtrContainer() : _ptr(NULL) {}

    explicit PtrContainer(T* obj) : _ptr(obj) {}

    ~PtrContainer() {
        delete _ptr;
    }

    PtrContainer(const PtrContainer& rhs)
        : _ptr(rhs._ptr ? new T(*rhs._ptr) : NULL) {}
    
    void operator=(const PtrContainer& rhs) {
        if (this == &rhs) {
            return;
        }

        if (rhs._ptr) {
            if (_ptr) {
                *_ptr = *rhs._ptr;
            } else {
                _ptr = new T(*rhs._ptr);
            }
        } else {
            delete _ptr;
            _ptr = NULL;
        }
    }

    T* get() const { return _ptr; }

    void reset(T* ptr) {
        delete _ptr;
        _ptr = ptr;
    }

    operator void*() const { return _ptr; }

    explicit operator bool() const { return get() != NULL; }

    T& operator*() const { return *get(); }

    T* operator->() const { return get(); }

private:
    T* _ptr;
};

}  // namespace mutil

#endif  // MUTIL_PTR_CONTAINER_H
