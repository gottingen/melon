//
// Created by liyinbin on 2021/4/19.
//

#ifndef ABEL_IO_INTERNAL_IOBUF_BASE_H_
#define ABEL_IO_INTERNAL_IOBUF_BASE_H_


#include <atomic>
#include <chrono>
#include <limits>
#include <utility>

#include "abel/io/internal/single_linked_list.h"
#include "abel/memory/object_pool.h"
#include "abel/memory/ref_ptr.h"

namespace abel {

    namespace io_internal {

        struct iobuf_block_deleter;

    }  // namespace io_internal

    class iobuf_block
            : public abel::ref_counted<iobuf_block,
                    io_internal::iobuf_block_deleter> {
    public:
        virtual ~iobuf_block() = default;

        virtual const char *data() const noexcept = 0;

        virtual std::size_t size() const noexcept = 0;

        virtual void destroy() noexcept { delete this; }
    };


    class iobuf_slice {
    public:
        iobuf_slice() = default;

        iobuf_slice(const iobuf_slice &) = default;

        iobuf_slice &operator=(const iobuf_slice &) = default;

        iobuf_slice(iobuf_slice &&other) noexcept
                : ptr_(other.ptr_), size_(other.size_), ref_(std::move(other.ref_)) {
            other.clear();
        }

        iobuf_slice &operator=(iobuf_slice &&other) noexcept {
            if (this != &other) {
                ptr_ = other.ptr_;
                size_ = other.size_;
                ref_ = std::move(other.ref_);
                other.clear();
            }
            return *this;
        }

        // Same as constructing a new one and call `Reset` on it.
        iobuf_slice(abel::ref_ptr <iobuf_block> data, std::size_t start,
                    std::size_t size)
                : ptr_(data->data() + start), size_(size), ref_(std::move(data)) {}

        // Accessor.
        const char *data() const noexcept { return ptr_; }

        std::size_t size() const noexcept { return size_; }

        // Changes the portion of buffer we're seeing.
        void skip(std::size_t bytes) {
            DCHECK_LT(bytes, size_);
            size_ -= bytes;
            ptr_ += bytes;
        }

        void set_size(std::size_t size) {
            DCHECK_LE(size, size_);
            size_ = size;
        }

        // Accepts a new buffer block.
        void reset(abel::ref_ptr <iobuf_block> data, std::size_t start,
                   std::size_t size) {
            DCHECK_LE(start, size);
            DCHECK_LE(size, data->size());
            ref_ = std::move(data);
            ptr_ = ref_->data() + start;
            size_ = size;
        }

        // Resets everything.
        void clear() {
            ptr_ = nullptr;
            size_ = 0;
            ref_ = nullptr;
        }

    private:
        friend class iobuf;

        io_internal::single_linked_list_entry chain;
        const char *ptr_{};
        std::size_t size_{};
        abel::ref_ptr <iobuf_block> ref_;
    };

    namespace io_internal {

        struct iobuf_block_deleter {
            void operator()(iobuf_block *p) {
                DCHECK_EQ(p->unsafe_ref_count(), 0);
                p->ref_count_.store(1, std::memory_order_relaxed);  // Rather hacky.
                p->destroy();
            }
        };
    }  // namespace io_internal

}  // namespace abel

namespace abel {

    template<>
    struct pool_traits<abel::iobuf_slice> {
        static constexpr auto kType = pool_type::ThreadLocal;
        static constexpr auto kLowWaterMark = 32768;
        static constexpr auto kHighWaterMark =
                std::numeric_limits<std::size_t>::max();
        static constexpr auto kMaxIdle = abel::duration::seconds(10);
        static constexpr auto kMinimumThreadCacheSize = 8192;
        static constexpr auto kTransferBatchSize = 1024;

        static void OnPut(abel::iobuf_slice *bb) {
            bb->clear();  // We don't need the data to be kept.
        }
    };

}  // namespace abel

#endif  // ABEL_IO_INTERNAL_IOBUF_BASE_H_
