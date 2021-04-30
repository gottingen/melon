//
// Created by liyinbin on 2021/4/19.
//

#ifndef ABEL_IO_IOBUF_H_
#define ABEL_IO_IOBUF_H_

#include <cstddef>

#include <algorithm>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "abel/io/internal/iobuf_block.h"
#include "abel/io/internal/iobuf_base.h"
#include "abel/io/internal/single_linked_list.h"
#include "abel/log/logging.h"

namespace abel {

    namespace io_internal {

        template<class T>
        constexpr auto data(const T &c)
        -> std::enable_if_t<std::is_convertible_v<decltype(c.data()), const char *>,
                const char *> {
            return c.data();
        }

        template<std::size_t N>
        constexpr auto data(const char (&c)[N]) {
            return c;
        }

        template<class T>
        constexpr std::size_t size(const T &c) {
            return c.size();
        }

        // Similar to `std::size` except for it returns number of chars instead of array
        // size (which is 1 greater than number of chars.)
        template<std::size_t N>
        constexpr std::size_t size(const char (&c)[N]) {
            if (N == 0) {
                return 0;
            }
            return N - (c[N - 1] == 0);
        }

    }  // namespace io_internal

    // Internally a `iobuf` consists of multiple `iobuf_slice`s.
    class iobuf {
        using LinkedBuffers =
        io_internal::single_linked_list<iobuf_slice, &iobuf_slice::chain>;

    public:
        using iterator = LinkedBuffers::iterator;
        using const_iterator = LinkedBuffers::const_iterator;

        constexpr iobuf() = default;

        // Copyable & movable.
        //
        // It's relatively cheap to copy this object but move still performs better.
        iobuf(const iobuf &nb);

        iobuf &operator=(const iobuf &nb);

        iobuf(iobuf &&nb) noexcept
                : byte_size_(std::exchange(nb.byte_size_, 0)),
                  buffers_(std::move(nb.buffers_)) {}

        iobuf &operator=(iobuf &&nb) noexcept {
            if (ABEL_UNLIKELY(&nb == this)) {
                return *this;
            }
            clear();
            std::swap(byte_size_, nb.byte_size_);
            buffers_ = std::move(nb.buffers_);
            return *this;
        }

        ~iobuf() { clear(); }

        // Returns first "contiguous" part of this buffer.
        //
        // Precondition: !empty().
        std::string_view first_slice() const noexcept {
            DCHECK(!empty());
            auto &&first = buffers_.front();
            return std::string_view(first.data(), first.size());
        }

        // `bytes` can be greater than `first_slice()->size()`, in this case
        // multiple buffer blocks are dropped.
        void skip(std::size_t bytes) noexcept {
            DCHECK_LE(bytes, byte_size());
            if (ABEL_UNLIKELY(bytes == 0)) {
                // NOTHING.
            } else if (bytes < buffers_.front().size()) {
                buffers_.front().skip(bytes);
                byte_size_ -= bytes;
            } else {
                skip_slow(bytes);
            }
        }

        // Cut off first `bytes` bytes. That is, the first `bytes` bytes are removed
        // from `*this` and returned to the caller. `bytes` could be larger than
        // `first_slice().size()`.
        //
        // `bytes` may not be greater than `byte_size()`. Otherwise the behavior is
        // undefined.
        //
        // FIXME: Exception safety.
        iobuf cut(std::size_t bytes) {
            DCHECK_LE(bytes, byte_size());

            iobuf rc;
            auto left = bytes;

            while (left && left >= buffers_.front().size()) {
                left -= buffers_.front().size();
                rc.buffers_.push_back(buffers_.pop_front());
            }

            if (ABEL_LIKELY(left)) {
                auto ncb =
                        abel::object_pool::get<iobuf_slice>().leak();  // Exception unsafe.
                *ncb = buffers_.front();
                ncb->set_size(left);
                rc.buffers_.push_back(ncb);
                buffers_.front().skip(left);
            }
            rc.byte_size_ = bytes;
            byte_size_ -= bytes;
            return rc;
        }

        void append(iobuf_slice buffer) {
            if ((buffer.size() == 0)) {  // Why would you do this?
                return;
            }
            auto block = abel::object_pool::get<iobuf_slice>();
            *block = std::move(buffer);
            byte_size_ += block->size();
            buffers_.push_back(block.leak());
        }

        void append(iobuf buffer) {
            byte_size_ += std::exchange(buffer.byte_size_, 0);
            buffers_.splice(std::move(buffer.buffers_));
        }

        // Total size of all buffers blocks.
        std::size_t byte_size() const noexcept { return byte_size_; }

        bool empty() const noexcept {
            DCHECK_EQ(buffers_.empty(), !byte_size_);
            return !byte_size_;
        }

        void clear() noexcept {
            if (!empty()) {
                clear_slow();
            }
        }

        // Non-mutating traversal.
        //
        // It's guaranteed that all elements are non-empty (i.e, their sizes are all
        // non-zero.).
        auto begin() const noexcept { return buffers_.begin(); }

        auto end() const noexcept { return buffers_.end(); }

    private:
        void skip_slow(std::size_t bytes) noexcept;

        void clear_slow() noexcept;

    private:
        std::size_t byte_size_{};
        LinkedBuffers buffers_;
    };

    // This class builds `iobuf`.
    class iobuf_builder {
        // If `append` is called with a buffer smaller than this threshold, it might
        // get copied even if technically a zero-copy mechanism is possible.
        //
        // This helps reduce internal memory fragmentation.
        inline static constexpr auto kAppendViaCopyThreshold = 128;

    public:
        iobuf_builder() { initialize_next_block(); }

        // Get a pointer for writing. It's size is available at `size_available()`.
        char *data() const noexcept { return current_->mutable_data() + used_; }

        // Space available in buffer returned by `data()`.
        std::size_t size_available() const noexcept {
            return current_->size() - used_;
        }

        // Mark `bytes` bytes as written (i.e., advance `data()` and reduce `size()`).
        //
        // New internal buffer is allocated if the current one is saturated (i.e,
        // `bytes` == `size()`.).
        void mark_written(std::size_t bytes) {
            DCHECK_MSG(bytes <= size_available(), "You're overflowing the buffer.");
            used_ += bytes;
            if (ABEL_UNLIKELY(!size_available())) {
                flush_current_block();
                initialize_next_block();
            }
        }

        // Reserve a contiguous block of bytes to be overwritten later.
        //
        // Maximum contiguous buffer block size is dynamically determined by a GFlag.
        // To be safe, you should never reserve more than 1K bytes.
        //
        // This method is provided for advance users.
        //
        // Pointer to the beginning of reserved block is returned.
        char *reserve(std::size_t bytes) {
            static const auto kMaxBytes = 1024;

            DCHECK_MSG(bytes <= kMaxBytes,
                       "At most [{}] bytes may be reserved in a single call.",
                       kMaxBytes);
            if (size_available() < bytes) {  // No enough contiguous buffer space,
                // allocated a new one then.
                flush_current_block();
                initialize_next_block();
            }
            auto ptr = data();
            mark_written(bytes);
            return ptr;
        }

        // Total number of bytes written.
        std::size_t byte_size() const noexcept { return nb_.byte_size() + used_; }

        // Clean up internal state and move buffer built out.
        //
        // CAUTION: You may not touch the builder after calling this method.
        iobuf destructive_get() noexcept {
            flush_current_block();
            return std::move(nb_);
        }

        // Some helper methods for building buffer more conveniently go below.

        // append `length` bytes from `ptr` into its internal buffer.
        void append(const void *ptr, std::size_t length) {
            // We speculatively increase `used_` here. This may cause it to temporarily
            // overflow. In that unlikely case, we failback to revert the change and
            // call `append_slow` instead.
            auto current = data();
            used_ += length;
            // If `used_` equals to buffer block size, we CANNOT use the optimization,
            // as a new block need to be allocated (which is done by `append_slow()`).
            if (ABEL_LIKELY(used_ < current_->size())) {
                // GCC 10 reports a false positive here.
#if __GNUC__ == 10
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
                memcpy(current, ptr, length);
#if __GNUC__ == 10
#pragma GCC diagnostic pop
#endif
                return;
            }
            used_ -= length;  // Well we failed..
            append_slow(ptr, length);
        }

        // append `s` to its internal buffer.
        void append(const std::string_view &s) { return append(s.data(), s.size()); }

        void append(iobuf_slice buffer) {
            // If the `buffer` is small enough, and append it to the current block does
            // not introduce new block, copying it can help in reducing internal
            // fragmentation.
            if (buffer.size() < kAppendViaCopyThreshold &&
                current_->size() - used_ >= buffer.size()) {
                append(buffer.data(), buffer.size());
                return;
            }
            // If there's nothing in our internal buffer, we don't need to flush it.
            if (used_) {
                flush_current_block();
                initialize_next_block();
            }
            nb_.append(std::move(buffer));
        }

        void append(iobuf buffer) {
            if (buffer.byte_size() < kAppendViaCopyThreshold &&
                current_->size() - used_ >= buffer.byte_size()) {
                append_copy(buffer);
                return;
            }
            if (used_) {
                flush_current_block();
                initialize_next_block();
            }
            nb_.append(std::move(buffer));
        }

        // append char `c` to its internal buffer.
        void append(char c) { append(static_cast<unsigned char>(c)); }

        void append(unsigned char c) {
            DCHECK(size_available());  // We never left internal buffer full.
            *reinterpret_cast<unsigned char *>(data()) = c;
            mark_written(1);
        }

        // If you want to append several **small** buffers that are **unlikely** to
        // cause new buffer block to be allocated, this method saves some arithmetic
        // operations.
        //
        // `Ts::data()` & `Ts::size()` must be available for the type to be used with
        // this method.
        template<class... Ts,
                class = std::void_t<decltype(io_internal::data(std::declval<Ts &>()))...>,
                class = std::void_t<decltype(io_internal::size(std::declval<Ts &>()))...>,
                class = std::enable_if_t<(sizeof...(Ts) > 1)>>
        void append(const Ts &... buffers) {
            auto current = data();
            auto total = (io_internal::size(buffers) + ...);
            used_ += total;
            if (ABEL_LIKELY(used_ < current_->size())) {
                unchecked_append(current, buffers...);
                return;
            }

            used_ -= total;
            // Initializers are evaluated in order, so order of the buffers is kept.
            [[maybe_unused]] int dummy[] = {(append(buffers), 0)...};
        }


    private:
        // Allocate a new buffer.
        void initialize_next_block();

        // Move the buffer block we're working on into the non-contiguous buffer we're
        // building.
        void flush_current_block();

        // Slow case for `append`.
        void append_slow(const void *ptr, std::size_t length);

        // Copy `buffer` into us. This is an optimization for small `buffer`s, so as
        // to reduce internal fragmentation.
        void append_copy(const iobuf &buffer);

        template<class T, class... Ts>
        [[gnu::always_inline]] void unchecked_append(char *ptr, const T &sv) {
            memcpy(ptr, abel::detail::data(sv), io_internal::size(sv));
        }

        template<class T, class... Ts>
        [[gnu::always_inline]] void unchecked_append(char *ptr, const T &sv,
                                                    const Ts &... svs) {
            memcpy(ptr, data(sv), io_internal::size(sv));
            unchecked_append(ptr + io_internal::size(sv), svs...);
        }

    private:
        iobuf nb_;
        std::size_t used_;
        abel::ref_ptr<native_iobuf_block> current_;
    };

// Helper functions go below.

    namespace io_internal {

        void flatten_to_slow_slow(const iobuf &nb, void *buffer,
                               std::size_t size);

    }  // namespace io_internal

    iobuf create_buffer_slow(std::string_view s);

    iobuf create_buffer_slow(const void *ptr, std::size_t size);

    std::string flatten_slow(
            const iobuf &nb,
            std::size_t max_bytes = std::numeric_limits<std::size_t>::max());

    // `delim` is included in the result string.
    std::string flatten_slow_until(
            const iobuf &nb, std::string_view delim,
            std::size_t max_bytes = std::numeric_limits<std::size_t>::max());

    // Caller is responsible for ensuring `nb.byte_size()` is no less than `size`.
    inline void flatten_to_slow(const iobuf &nb, void *buffer,
                              std::size_t size) {
        if (ABEL_LIKELY(size <= nb.first_slice().size())) {
            memcpy(buffer, nb.first_slice().data(), size);
        }
        return io_internal::flatten_to_slow_slow(nb, buffer, size);
    }

    // Make a buffer block that references to memory region pointed to by `buffer`.
    //
    // It's your responsibility to make sure memory regions referenced are valid and
    // not mutated until the resulting buffer is consumed (i.e., destroyed).
    iobuf_slice make_ref_slice(const void *ptr, std::size_t size);

    // This overload accepts a completion callback. This provide a way for the
    // creator to be notified when the framework finished using the buffer.
    template<class F>
    iobuf_slice make_ref_slice(const void *ptr, std::size_t size,
                                      F &&completion_cb) {
        using BufferBlock = ref_iobuf_block<std::remove_reference_t<F>>;
        return iobuf_slice(
                abel::make_ref_counted<BufferBlock>(ptr, size, std::forward<F>(completion_cb)), 0, size);
    }

    // Create a buffer that owns the container passed to this method.
    iobuf_slice make_foreign_slice(std::string buffer);

    // Create a buffer that owns `buffer`.
    //
    // `T` may only be one of the following types:
    //
    // - `std::byte`.
    // - Builtin integral types.
    // - Builtin floating-point types.
    template<class T>
    iobuf_slice make_foreign_slice(std::vector<T> buffer);

}  //namespace abel

#endif  // ABEL_IO_IOBUF_H_
