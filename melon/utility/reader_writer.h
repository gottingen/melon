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


// Date: Wed Aug  8 05:51:33 PDT 2018

#ifndef MUTIL_READER_WRITER_H
#define MUTIL_READER_WRITER_H

#include <sys/uio.h>                             // iovec

namespace mutil {

// Abstraction for reading data.
// The simplest implementation is to embed a file descriptor and read from it.
class IReader {
public:
    virtual ~IReader() {}

    // Semantics of parameters and return value are same as readv(2) except that
    // there's no `fd'.
    virtual ssize_t ReadV(const iovec* iov, int iovcnt) = 0;
};

// Abstraction for writing data.
// The simplest implementation is to embed a file descriptor and writev into it.
class IWriter {
public:
    virtual ~IWriter() {}

    // Semantics of parameters and return value are same as writev(2) except that
    // there's no `fd'.
    // WriteV is required to submit data gathered by multiple appends in one 
    // run and enable the possibility of atomic writes.
    virtual ssize_t WriteV(const iovec* iov, int iovcnt) = 0;
};

}  // namespace mutil

#endif  // MUTIL_READER_WRITER_H
