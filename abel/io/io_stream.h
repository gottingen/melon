//
// Created by liyinbin on 2021/5/1.
//

#ifndef ABEL_IO_IO_STREAM_H_
#define ABEL_IO_IO_STREAM_H_

#include <sys/uio.h>
#include <cstddef>

#include "abel/io/safe_io.h"

namespace abel {

    class io_stream_base {
    public:
        virtual ~io_stream_base() = default;

        enum class hand_shake_status {
            eSuccess,
            eRead,
            eWrite,
            eError
        };

        virtual hand_shake_status handshake() = 0;


        virtual ssize_t readv(const iovec *iov, int iovcnt) = 0;

        virtual ssize_t writev(const iovec *iov, int iovcnt) = 0;
    };

    class system_io_stream : public io_stream_base {
    public:
        explicit system_io_stream(int fd) : _fd(fd) {}

        hand_shake_status handshake() override {
            return hand_shake_status::eSuccess;
        }

        ssize_t readv(const iovec *iov, int iovcnt) override {
            return abel::safe_readv(_fd, iov, iovcnt);
        }

        ssize_t writev(const iovec *iov, int iovcnt) override {
            return abel::safe_writev(_fd, iov, iovcnt);
        }

        int get_fd() const {
            return _fd;
        }

    private:
        const int _fd;
    };
}  // namespace abel

#endif  // ABEL_IO_IO_STREAM_H_
