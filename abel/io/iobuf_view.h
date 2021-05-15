//
// Created by liyinbin on 2021/4/19.
//

#ifndef ABEL_IOBUF_VIEW_H
#define ABEL_IOBUF_VIEW_H


#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

#include "abel/io/iobuf.h"
#include "abel/memory/non_destroy.h"

namespace abel {

    // This class provides a visually contiguous byte-wise view of a buffer.
    //
    // Performance note: Due to implementation details, scanning through a buffer
    // via this class is much slower than scanning the buffer in a non-contiguous
    // fashion.
    //
    // A `ForwardIterator` is returned. Random access is not supported.
    class iobuf_forward_view {
    public:
        class const_iterator;

        using iterator = const_iterator;

        iobuf_forward_view() = default;

        explicit iobuf_forward_view(const iobuf &buffer)
                : buffer_(&buffer) {}

        const_iterator begin() const noexcept;

        const_iterator end() const noexcept;

        bool empty() const noexcept { return buffer_->empty(); }

        std::size_t size() const noexcept { return buffer_->byte_size(); }

    private:
        const iobuf *buffer_ ;//=
               // &abel::early_init_constant<iobuf>();
    };

    // This class provides random access into a buffer.
    //
    // Internally this class build a mapping of all discontiguous buffer blocks.
    // This comes at a cost. Therefore, unless you absolutely need it, stick with
    // "forward" view.
    //
    // Same performance consideration as of `iobuf_forward_view` also
    // applies here.
    class iobuf_view {
    public:
        class const_iterator;

        using iterator = const_iterator;

        iobuf_view();

        explicit iobuf_view(const iobuf &buffer);

        // Random access. This is slower than traversal.
        char operator[](std::size_t offset) const noexcept {
            DCHECK_LT(offset, size());
            auto&&[off, iter] = find_segment_must_succeed(offset);
            return iter->data()[offset - off];
        }

        const_iterator begin() const noexcept;

        const_iterator end() const noexcept;

        bool empty() const noexcept { return byte_size_ == 0; }

        std::size_t size() const noexcept { return byte_size_; }

    private:
        std::pair<std::size_t, iobuf::const_iterator>
        find_segment_must_succeed(std::size_t offset) const noexcept;

    private:
        std::size_t byte_size_ = 0;
        // [Starting offset -> iterator].
        std::vector<std::pair<std::size_t, iobuf::const_iterator>>
                offsets_;
    };

///////////////////////////////////////
// Implementation goes below.        //
///////////////////////////////////////

    class iobuf_forward_view::const_iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = char;
        using pointer = const char *;
        using reference = const char &;
        using iterator_category = std::forward_iterator_tag;

        const_iterator() = default;  // `end()`.

        char operator*() const noexcept {
            DCHECK(current_ != end_, "Dereferencing an invalid iterator.");
            DCHECK_LT(byte_offset_, current_->size());
            return *(current_->data() + byte_offset_);
        }

        const_iterator &operator++() noexcept {
            DCHECK(current_ != end_);
            DCHECK(byte_offset_ < current_->size());
            ++byte_offset_;
            if (ABEL_UNLIKELY(byte_offset_ == current_->size())) {
                byte_offset_ = 0;
                ++current_;
            }
            return *this;
        }

        bool operator==(const const_iterator &iter) const noexcept {
            return current_ == iter.current_ && byte_offset_ == iter.byte_offset_;
        }

        bool operator!=(const const_iterator &iter) const noexcept {
            return current_ != iter.current_ || byte_offset_ != iter.byte_offset_;
        }

    private:
        friend class iobuf_forward_view;

        const_iterator(iobuf::const_iterator begin,
                       iobuf::const_iterator end)
                : current_(begin), end_(end), byte_offset_(0) {}

    private:
        iobuf::const_iterator current_, end_;
        std::size_t byte_offset_;
    };

    inline iobuf_forward_view::const_iterator
    iobuf_forward_view::begin() const noexcept {
        return const_iterator(buffer_->begin(), buffer_->end());
    }

    inline iobuf_forward_view::const_iterator
    iobuf_forward_view::end() const noexcept {
        return const_iterator();
    }

    class iobuf_view::const_iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = char;
        using pointer = const char *;
        using reference = const char &;
        using iterator_category = std::forward_iterator_tag;

        const_iterator() = default;  // `end()`.

        char operator*() const noexcept {
            DCHECK(current_ != end_, "Dereferencing an invalid iterator.");
            DCHECK_LT(seg_offset_, current_->size());
            return *(current_->data() + seg_offset_);
        }

        const_iterator &operator+=(std::ptrdiff_t offset) noexcept {
            SeekTo(offset);
            return *this;
        }

        const_iterator operator+(std::ptrdiff_t offset) noexcept {
            auto copy(*this);
            copy += offset;
            return copy;
        }

        std::ptrdiff_t operator-(const const_iterator &other) const noexcept {
            return byte_offset_ - other.byte_offset_;
        }

        const_iterator &operator++() noexcept {
            DCHECK(current_ != end_);
            DCHECK(seg_offset_ < current_->size());
            ++seg_offset_;
            if (ABEL_UNLIKELY(seg_offset_ == current_->size())) {
                seg_offset_ = 0;
                ++current_;
            }
            ++byte_offset_;
            return *this;
        }

        bool operator==(const const_iterator &iter) const noexcept {
            DCHECK_EQ(view_, iter.view_);
            return byte_offset_ == iter.byte_offset_;
        }

        bool operator!=(const const_iterator &iter) const noexcept {
            DCHECK_EQ(view_, iter.view_);
            return byte_offset_ != iter.byte_offset_;
        }

    private:
        friend class iobuf_view;

        explicit const_iterator(const iobuf_view *view)
                : view_(view) {
            byte_offset_ = seg_offset_ = 0;
            current_ = view->offsets_.front().second;
            end_ = view->offsets_.back().second;
        }

        void SeekTo(std::size_t offset) noexcept {
            DCHECK_LE(offset, view_->size());
            auto&&[off, iter] = view_->find_segment_must_succeed(offset);
            byte_offset_ = offset;
            seg_offset_ = offset - off;
            current_ = iter;
        }

    private:
        const iobuf_view *view_;
        std::size_t byte_offset_;
        iobuf::const_iterator current_, end_;
        std::size_t seg_offset_;
    };

    inline iobuf_view::const_iterator
    iobuf_view::begin() const noexcept {
        return const_iterator(this);
    }

    inline iobuf_view::const_iterator
    iobuf_view::end() const noexcept {
        const_iterator result(this);
        result.SeekTo(size());
        return result;
    }

}  // namespace abel

#endif //ABEL_IOBUF_VIEW_H
