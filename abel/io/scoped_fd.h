//
// Created by liyinbin on 2020/3/30.
//

#ifndef ABEL_IO_SCOPED_FD_H_
#define ABEL_IO_SCOPED_FD_H_

#include <unistd.h>
#include <abel/base/profile.h>

namespace abel {

    class scoped_fd {
    public:
        scoped_fd() : _fd(-1) {}

        explicit scoped_fd(int fd) : _fd(fd) {}

        ~scoped_fd() {
            if (_fd >= 0) {
                ::close(_fd);
                _fd = -1;
            }
        }

        void reset(int fd) {
            if (_fd >= 0) {
                ::close(_fd);
                _fd = -1;
            }
            _fd = fd;
        }

        int release() {
            const int prev_fd = _fd;
            _fd = -1;
            return prev_fd;
        }

        operator int() const { return _fd; }

    private:
        ABEL_NON_COPYABLE(scoped_fd);
        int _fd;
    };


} //namespace abel

#endif //ABEL_IO_SCOPED_FD_H_
