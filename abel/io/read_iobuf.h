//
// Created by liyinbin on 2021/5/1.
//

#ifndef ABEL_IO_READ_IOBUF_H_
#define ABEL_IO_READ_IOBUF_H_

#include "abel/io/iobuf.h"
#include "abel/io/io_stream.h"

namespace abel {

    enum class read_status {
        eDrained,

        eMaxBytesRead,

        eEof,

        eError
    };

    read_status read_iobuf(std::size_t max_bytes, io_stream_base* io,
                          iobuf* to, std::size_t* bytes_read);

}  // namespace abel

#endif  // ABEL_IO_READ_IOBUF_H_
