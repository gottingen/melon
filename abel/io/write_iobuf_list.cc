//
// Created by liyinbin on 2021/5/1.
//

#include "abel/io/write_iobuf_list.h"

#include <limits.h>
#include <algorithm>
#include <limits>
#include <utility>
#include "abel/chrono/duration.h"
#include "abel/memory/object_pool.h"
#include "abel/log/logging.h"

namespace abel {

    template<>
    struct pool_traits<write_iobuf_list::list_node> {
        static constexpr auto kType = pool_type::ThreadLocal;
        static constexpr auto kLowWaterMark = 8192;
        static constexpr auto kHighWaterMark =
                std::numeric_limits<std::size_t>::max();
        static constexpr auto kMaxIdle = abel::duration::seconds(10);
        static constexpr auto kMinimumThreadCacheSize = 2048;
        static constexpr auto kTransferBatchSize = 2048;

        static void OnPut(write_iobuf_list::list_node *p) {
            p->buffer.clear();
        }
    };

    /// write_iobuf_list


    write_iobuf_list::write_iobuf_list() {
        _tail.store(nullptr, std::memory_order_relaxed);
        // `_head` is not initialized here.
        //
        // Each time `_tail` is reset to `nullptr`, `_head` will be initialized by the
        // next call to `Append`.
    }

    write_iobuf_list::~write_iobuf_list() {
        // Update `_head` in case it's in inconsistent state (`flust` can leave it
        // in such a state and leave it for `append` to fix.).
        append({}, {});

        // Free the list.
        auto current = _head.load(std::memory_order_acquire);
        while (current) {
            pooled_ptr<list_node> ptr(current);  // To be freed.
            current = current->next.load(std::memory_order_acquire);
        }
    }

    ssize_t write_iobuf_list::flush(io_stream_base *io, std::size_t max_bytes,
                                    std::vector<std::uintptr_t> *flushed_ctxs,
                                    bool *emptied, bool *short_write) {

        thread_local iovec iov[IOV_MAX];

        std::size_t nv = 0;
        std::size_t flushing = 0;

        auto head = _head.load(std::memory_order_acquire);
        auto current = head;
        DCHECK(current);  // It can't be. `Append` should have already updated
        // it.
        DCHECK(_tail.load(std::memory_order_relaxed), "The buffer is empty.");
        while (current) {
            for (auto iter = current->buffer.begin();
                 iter != current->buffer.end() && nv != std::size(iov) &&
                 flushing < max_bytes;
                 ++iter) {
                auto &&e = iov[nv++];
                e.iov_base = const_cast<char *>(iter->data());
                e.iov_len = iter->size();  // For the last iov, we revise its size later.

                flushing += e.iov_len;
            }
            current = current->next.load(std::memory_order_acquire);
        }

        if (ABEL_LIKELY(flushing > max_bytes)) {
            auto diff = flushing - max_bytes;
            iov[nv - 1].iov_len -= diff;
            flushing -= diff;
        }

        ssize_t rc = io->writev(iov, nv);
        if (rc <= 0) {
            return rc;  // Nothing is really flushed then.
        }
        DCHECK_LE(static_cast<size_t>(rc), flushing);

        // We did write something out. Remove those buffers and update the result
        // accordingly.
        auto flushed = static_cast<std::size_t>(rc);
        bool drained = false;

        // Rewind.
        //
        // We do not have to reload `_head`, it shouldn't have changed.
        current = head;
        while (current) {
            if (auto b = current->buffer.byte_size(); b <= flushed) {
                // The entire buffer was written then.
                pooled_ptr destroying(current);  // To be freed.

                flushed -= b;
                flushed_ctxs->push_back(current->ctx);
                if (auto next = current->next.load(std::memory_order_acquire); !next) {
                    DCHECK_EQ(0, flushed);  // Or we have written out more than what we
                    auto expected_tail = current;
                    if (_tail.compare_exchange_strong(expected_tail, nullptr,
                                                      std::memory_order_relaxed)) {
                        // We successfully marked the list as empty.
                        drained = true;
                        // `_head` will be reset by next `Append`.
                    } else {
                        list_node *ptr;
                        do {
                            ptr = current->next.load(std::memory_order_acquire);
                        } while (!ptr);
                        _head.store(ptr, std::memory_order_release);
                    }
                    // In either case, we've finished rewinding, up to where we've written
                    // out.
                    break;
                } else {
                    // Move to the next one.
                    current = next;
                }
            } else {
                current->buffer.skip(flushed);
                // We didn't drain the list, set `_head` to where we left off.
                _head.store(current, std::memory_order_release);
                break;
            }
        }

        *emptied = drained;
        *short_write = (static_cast<size_t>(rc) != flushing);
        return rc;
    }

    bool write_iobuf_list::append(iobuf buffer, std::uintptr_t ctx) {
        auto node = object_pool::get<list_node>();

        node->next.store(nullptr, std::memory_order_relaxed);
        node->buffer = std::move(buffer);
        node->ctx = ctx;

        // By an atomic exchange between `_tail` and `node`, we atomically set `node`
        // as the new tail.
        auto prev = _tail.exchange(node.get(), std::memory_order_acq_rel);
        if (!prev) {
            _head.store(node.get(), std::memory_order_release);
        } else {
            // Otherwise there was a node (the old tail), so set us as its successor.
            DCHECK(!prev->next.load(std::memory_order_acquire));
            prev->next.store(node.get(), std::memory_order_release);
        }
        (void) node.leak();  // It will be freed on deque.

        return !prev;  // We changed `_head`.
    }

}  // namespace abel
