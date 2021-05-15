//
// Created by liyinbin on 2021/4/19.
//


#include "abel/io/iobuf.h"

#include <algorithm>
#include <utility>

#include "abel/log/logging.h"

namespace abel {

    namespace {

        template<class T>
        class owningiobuf_block : public iobuf_block {
        public:
            explicit owningiobuf_block(T storage) : storage_(std::move(storage)) {}

            const char *data() const noexcept override {
                return reinterpret_cast<const char *>(storage_.data());
            }

            std::size_t size() const noexcept override { return storage_.size(); }

        private:
            T storage_;
        };

    }  // namespace

    namespace detail {

        void flatten_to_slow_slow(const iobuf &nb, void *buffer,
                                  std::size_t size) {
            DCHECK(nb.byte_size() >= size, "No enough data.");
            std::size_t copied = 0;
            for (auto iter = nb.begin(); iter != nb.end() && copied != size; ++iter) {
                auto len = std::min(size - copied, iter->size());
                memcpy(reinterpret_cast<char *>(buffer) + copied, iter->data(), len);
                copied += len;
            }
        }

    }  // namespace detail

    iobuf::iobuf(const iobuf &nb)
            : byte_size_(nb.byte_size_) {
        for (auto &&e : nb.buffers_) {
            // Exception unsafe.
            auto b = abel::object_pool::get<iobuf_slice>().leak();
            *b = e;
            buffers_.push_back(b);
        }
    }

    iobuf &iobuf::operator=(
            const iobuf &nb) {
        if (ABEL_UNLIKELY(&nb == this)) {
            return *this;
        }
        clear();
        byte_size_ = nb.byte_size_;
        for (auto &&e : nb.buffers_) {
            // Exception unsafe.
            auto b = abel::object_pool::get<iobuf_slice>().leak();
            *b = e;
            buffers_.push_back(b);
        }
        return *this;
    }

    void iobuf::skip_slow(std::size_t bytes) noexcept {
        byte_size_ -= bytes;

        while (bytes) {
            auto &&first = buffers_.front();
            auto os = std::min(bytes, first.size());
            if (os == first.size()) {
                abel::object_pool::put<iobuf_slice>(buffers_.pop_front());
            } else {
                DCHECK_LT(os, first.size());
                first.skip(os);
            }
            bytes -= os;
        }
    }

    void iobuf::clear_slow() noexcept {
        byte_size_ = 0;
        while (!buffers_.empty()) {
            abel::object_pool::put<iobuf_slice>(buffers_.pop_front());
        }
    }

    void iobuf_builder::initialize_next_block() {
        if (current_) {
            DCHECK(size_available());
            return;  // Nothing to do then.
        }
        current_ = make_native_ionuf_block();
        used_ = 0;
    }

    // Move the buffer block we're working on into the non-contiguous buffer we're
    // building.
    void iobuf_builder::flush_current_block() {
        if (!used_) {
            return;  // The current block is clean, no need to flush it.
        }
        nb_.append(iobuf_slice(std::move(current_), 0, used_));
        used_ = 0;  // Not strictly needed as it'll be re-initialized by
        // `initialize_next_block()` anyway.
    }

    void iobuf_builder::append_slow(const void *ptr,
                                    std::size_t length) {
        while (length) {
            auto copying = std::min(length, size_available());
            memcpy(data(), ptr, copying);
            mark_written(copying);
            ptr = static_cast<const char *>(ptr) + copying;
            length -= copying;
        }
    }

    void iobuf_builder::append_copy(const iobuf &buffer) {
        for (auto &&e : buffer) {
            append(e.data(), e.size());
        }
    }

    iobuf create_buffer_slow(std::string_view s) {
        iobuf_builder nbb;
        nbb.append(s);
        return nbb.destructive_get();
    }

    iobuf create_buffer_slow(const void *ptr, std::size_t size) {
        iobuf_builder nbb;
        nbb.append(ptr, size);
        return nbb.destructive_get();
    }

    std::string flatten_slow(const iobuf &nb, std::size_t max_bytes) {
        max_bytes = std::min(max_bytes, nb.byte_size());
        std::string rc;
        std::size_t left = max_bytes;
        rc.reserve(max_bytes);
        for (auto iter = nb.begin(); iter != nb.end() && left; ++iter) {
            auto len = std::min(left, iter->size());
            rc.append(iter->data(), len);
            left -= len;
        }
        return rc;
    }

    std::string flatten_slow_until(const iobuf &nb,
                                   std::string_view delim, std::size_t max_bytes) {
        if (nb.empty()) {
            return {};
        }

        // Given that our block is large enough, and the caller should not be
        // expecting too much data (since this method is slow), it's likely we even
        // don't have to fully copy the first block. So we optimize for this case.
        std::string_view current(nb.first_slice().data(),
                                 nb.first_slice().size());
        if (auto pos = current.find(delim); pos != std::string_view::npos) {
            auto expected_bytes = std::min(pos + delim.size(), max_bytes);
            return std::string(nb.first_slice().data(),
                               nb.first_slice().data() + expected_bytes);
        }

        // Slow path otherwise.
        std::string rc;
        for (auto iter = nb.begin(); iter != nb.end() && rc.size() < max_bytes;
             ++iter) {
            auto old_len = rc.size();
            rc.append(iter->data(), iter->size());
            if (auto pos = rc.find(delim, old_len - std::min(old_len, delim.size()));
                    pos != std::string::npos) {
                rc.erase(rc.begin() + pos + delim.size(), rc.end());
                break;
            }
        }
        if (rc.size() > max_bytes) {
            rc.erase(rc.begin() + max_bytes, rc.end());
        }
        return rc;
    }

    iobuf_slice make_ref_slice(const void *ptr, std::size_t size) {
        return make_ref_slice(ptr, size, [] {});
    }

    iobuf_slice make_foreign_slice(std::string buffer) {
        auto size = buffer.size();
        return iobuf_slice(
                abel::make_ref_counted<owningiobuf_block<std::string>>(std::move(buffer)), 0,
                size);
    }

    template<class T>
    iobuf_slice make_foreign_slice(std::vector<T> buffer) {
        auto size = buffer.size() * sizeof(T);
        return iobuf_slice(
                abel::make_ref_counted<owningiobuf_block<std::vector<T>>>(std::move(buffer)), 0,
                size);
    }

#define INSTANTIATE_MAKE_FOREIGN_BUFFER(type) \
  template iobuf_slice make_foreign_slice<type>(std::vector<type> buffer);

    INSTANTIATE_MAKE_FOREIGN_BUFFER(std::byte);

    INSTANTIATE_MAKE_FOREIGN_BUFFER(char);                // NOLINT
    INSTANTIATE_MAKE_FOREIGN_BUFFER(signed char);         // NOLINT
    INSTANTIATE_MAKE_FOREIGN_BUFFER(signed short);        // NOLINT
    INSTANTIATE_MAKE_FOREIGN_BUFFER(signed int);          // NOLINT
    INSTANTIATE_MAKE_FOREIGN_BUFFER(signed long);         // NOLINT
    INSTANTIATE_MAKE_FOREIGN_BUFFER(signed long long);    // NOLINT
    INSTANTIATE_MAKE_FOREIGN_BUFFER(unsigned char);       // NOLINT
    INSTANTIATE_MAKE_FOREIGN_BUFFER(unsigned short);      // NOLINT
    INSTANTIATE_MAKE_FOREIGN_BUFFER(unsigned int);        // NOLINT
    INSTANTIATE_MAKE_FOREIGN_BUFFER(unsigned long);       // NOLINT
    INSTANTIATE_MAKE_FOREIGN_BUFFER(unsigned long long);  // NOLINT
    INSTANTIATE_MAKE_FOREIGN_BUFFER(float);

    INSTANTIATE_MAKE_FOREIGN_BUFFER(double);

    INSTANTIATE_MAKE_FOREIGN_BUFFER(long double);

}  // namespace abel
