//
// Created by liyinbin on 2021/4/30.
//

#ifndef ABEL_IO_SAFE_IO_H_
#define ABEL_IO_SAFE_IO_H_


#include <errno.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include <utility>

#include "abel/base/profile.h"

namespace abel {

    namespace io_internal {
        template<class F>
        [[gnu::noinline]] auto safe_call_slow(F &&f) {
            // Slow path.
            while (true) {
                auto rc = std::forward<F>(f)();
                if (ABEL_LIKELY(rc >= 0 || errno != EINTR)) {
                    return rc;
                }
            }
        }

        // Fast path, likely to be inlined.
        template<class F>
        inline auto safe_call(F &&f) {
            auto rc = std::forward<F>(f)();
            if (ABEL_LIKELY(rc >= 0 || errno != EINTR)) {
                return rc;
            }

            return safe_call_slow(std::forward<F>(f));
        }
    }  // namespace io_internal


    inline ssize_t safe_read(int fd, void *buf, size_t count) {
        return io_internal::safe_call([&] { return ::read(fd, buf, count); });
    }


    inline ssize_t safe_write(int fd, const void *buf, size_t count) {
        return io_internal::safe_call([&] { return ::write(fd, buf, count); });
    }

    inline ssize_t safe_readv(int fd, const iovec *iov, int iovcnt) {
        return io_internal::safe_call([&] { return ::readv(fd, iov, iovcnt); });
    }

    inline ssize_t safe_writev(int fd, const iovec *iov, int iovcnt) {
        return io_internal::safe_call([&] { return ::writev(fd, iov, iovcnt); });
    }


}  // namespace abel

#endif  // ABEL_IO_SAFE_IO_H_
