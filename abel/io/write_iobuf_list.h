//
// Created by liyinbin on 2021/5/1.
//

#ifndef ABEL_IO_WRITE_IOBUF_LIST_H_
#define ABEL_IO_WRITE_IOBUF_LIST_H_

#include <vector>
#include <atomic>

#include "abel/base/profile.h"
#include "abel/io/iobuf.h"
#include "abel/io/io_stream.h"

namespace abel {

    template <class>
    struct pool_traits;

    class alignas(hardware_destructive_interference_size) write_iobuf_list {
    public:

        write_iobuf_list();
        ~write_iobuf_list();

        ssize_t flush(io_stream_base* io, std::size_t max_bytes,
                        std::vector<std::uintptr_t>* flushed_ctxs, bool* emptied,
                        bool* short_write);

        bool append(iobuf buffer, std::uintptr_t ctx);

    private:

        struct list_node {
            std::atomic<list_node*> next;
            iobuf buffer;
            std::uintptr_t ctx;
        };

        friend struct pool_traits<list_node>;

        alignas(hardware_destructive_interference_size) std::atomic<list_node*> _head;
        alignas(hardware_destructive_interference_size) std::atomic<list_node*> _tail;
    };


}  // namespace abel

#endif  // ABEL_IO_WRITE_IOBUF_LIST_H_
