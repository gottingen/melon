//
// Created by liyinbin on 2021/5/1.
//

#include  "abel/io/read_iobuf.h"
#include <limits.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "abel/memory/object_pool.h"
#include "abel/log/logging.h"

namespace abel {

    namespace io_internal {
        constexpr auto kMaxBlocksPerRead = 8;

        std::vector<ref_ptr<native_iobuf_block>> *refill_and_get_blocks() {
            thread_local std::vector<ref_ptr<native_iobuf_block>> cache;
            while (cache.size() < kMaxBlocksPerRead) {
                cache.push_back(make_native_ionuf_block());
            }
            return &cache;
        }

        ssize_t read_partial(std::size_t max_bytes, io_stream_base *io,
                             iobuf *to, bool *short_read) {
            auto &&block_cache = refill_and_get_blocks();
            iovec iov[kMaxBlocksPerRead];
            DCHECK_EQ(block_cache->size(), std::size(iov));

            std::size_t iov_elements = 0;
            std::size_t bytes_to_read = 0;
            while (bytes_to_read != max_bytes && iov_elements != kMaxBlocksPerRead) {
                auto &&iove = iov[iov_elements];
                // Use blocks from back to front. This helps when we removes used blocks
                // from the cache (popping from back of a vector is cheaper.).
                auto &&block = (*block_cache)[kMaxBlocksPerRead - 1 - iov_elements];
                auto len =
                        std::min(block->size(), max_bytes - bytes_to_read /* Bytes left */);

                iove.iov_base = block->mutable_data();
                iove.iov_len = len;
                bytes_to_read += len;
                ++iov_elements;
            }

            // Now perform read with `readv`.
            auto result = io->readv(iov, iov_elements);
            if (ABEL_UNLIKELY(result <= 0)) {
                return result;
            }
            DCHECK_LE(static_cast<size_t>(result) , bytes_to_read);
            *short_read = (static_cast<size_t>(result) != bytes_to_read);
            std::size_t bytes_left = result;

            // Remove used blocks from the cache and move them into `to`.
            while (bytes_left) {
                auto current = std::move(block_cache->back());
                auto len = std::min(bytes_left, current->size());
                to->append(iobuf_slice(std::move(current), 0, len));
                bytes_left -= len;
                block_cache->pop_back();
            }
            return result;
        }
    }  // namespace io_internal

    read_status read_iobuf(std::size_t max_bytes, io_stream_base *io,
                           iobuf *to, std::size_t *bytes_read) {
        auto bytes_left = max_bytes;
        *bytes_read = 0;
        while (bytes_left) {
            // Read from the socket.
            bool short_read = false;  // GCC 10 reports a spurious uninitialized-var.
            auto bytes_to_read = bytes_left;
            auto read = io_internal::read_partial(bytes_to_read, io, to, &short_read);
            if (ABEL_UNLIKELY(read == 0)) {  // The remote side closed the connection.
                return read_status::eEof;
            }
            if (ABEL_UNLIKELY(read < 0)) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return read_status::eDrained;
                } else {
                    return read_status::eError;
                }
            }
            DCHECK_LE(static_cast<size_t>(read), bytes_to_read);

            // Let's update the statistics.
            *bytes_read += read;
            bytes_left -= read;

            if (short_read) {
                DCHECK_LT(static_cast<size_t>(read), bytes_to_read);
                return read_status::eDrained;
            }
        }
        DCHECK_EQ(*bytes_read, max_bytes);
        return read_status::eMaxBytesRead;
    }

}  // namespace abel
