
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_IO_READER_WRITER_H_
#define MELON_IO_READER_WRITER_H_

#include <sys/uio.h>                             // iovec

namespace melon {

    // Abstraction for reading data.
    // The simplest implementation is to embed a file descriptor and read from it.
    class base_reader {
    public:
        virtual ~base_reader() {}

        // Semantics of parameters and return value are same as readv(2) except that
        // there's no `fd'.
        virtual ssize_t readv(const iovec *iov, int iovcnt) = 0;
    };

    // Abstraction for writing data.
    // The simplest implementation is to embed a file descriptor and writev into it.
    class base_writer {
    public:
        virtual ~base_writer() {}

        // Semantics of parameters and return value are same as writev(2) except that
        // there's no `fd'.
        // writev is required to submit data gathered by multiple appends in one
        // run and enable the possibility of atomic writes.
        virtual ssize_t writev(const iovec *iov, int iovcnt) = 0;
    };

}  // namespace melon

#endif  // MELON_IO_READER_WRITER_H_
