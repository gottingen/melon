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


// Date: Mon. Nov 7 14:47:36 CST 2011

#ifndef MUTIL_FD_GUARD_H
#define MUTIL_FD_GUARD_H

#include <unistd.h>                                  // close()

namespace mutil {

// RAII file descriptor.
//
// Example:
//    fd_guard fd1(open(...));
//    if (fd1 < 0) {
//        printf("Fail to open\n");
//        return -1;
//    }
//    if (another-error-happened) {
//        printf("Fail to do sth\n");
//        return -1;   // *** closing fd1 automatically ***
//    }
class fd_guard {
public:
    fd_guard() : _fd(-1) {}
    explicit fd_guard(int fd) : _fd(fd) {}
    
    ~fd_guard() {
        if (_fd >= 0) {
            ::close(_fd);
            _fd = -1;
        }
    }

    // Close current fd and replace with another fd
    void reset(int fd) {
        if (_fd >= 0) {
            ::close(_fd);
            _fd = -1;
        }
        _fd = fd;
    }

    // Set internal fd to -1 and return the value before set.
    int release() {
        const int prev_fd = _fd;
        _fd = -1;
        return prev_fd;
    }
    
    operator int() const { return _fd; }
    
private:
    // Copying this makes no sense.
    fd_guard(const fd_guard&);
    void operator=(const fd_guard&);
    
    int _fd;
};

}  // namespace mutil

#endif  // MUTIL_FD_GUARD_H
