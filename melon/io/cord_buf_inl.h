
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_IO_CORD_BUF_INL_H_
#define MELON_IO_CORD_BUF_INL_H_

void *fast_memcpy(void *__restrict dest, const void *__restrict src, size_t n);

namespace melon {

    inline ssize_t cord_buf::cut_into_file_descriptor(int fd, size_t size_hint) {
        return pcut_into_file_descriptor(fd, -1, size_hint);
    }

    inline ssize_t cord_buf::cut_multiple_into_file_descriptor(
            int fd, cord_buf *const *pieces, size_t count) {
        return pcut_multiple_into_file_descriptor(fd, -1, pieces, count);
    }

    inline ssize_t IOPortal::append_from_file_descriptor(int fd, size_t max_count) {
        return pappend_from_file_descriptor(fd, -1, max_count);
    }

    inline void IOPortal::return_cached_blocks() {
        if (_block) {
            return_cached_blocks_impl(_block);
            _block = NULL;
        }
    }

    inline void reset_block_ref(cord_buf::BlockRef &ref) {
        ref.offset = 0;
        ref.length = 0;
        ref.block = NULL;
    }

    inline cord_buf::cord_buf() {
        reset_block_ref(_sv.refs[0]);
        reset_block_ref(_sv.refs[1]);
    }

    inline cord_buf::cord_buf(const Movable &rhs) {
        _sv = rhs.value()._sv;
        new(&rhs.value()) cord_buf;
    }

    inline void cord_buf::operator=(const Movable &rhs) {
        clear();
        _sv = rhs.value()._sv;
        new(&rhs.value()) cord_buf;
    }

    inline void cord_buf::operator=(const char *s) {
        clear();
        append(s);
    }

    inline void cord_buf::operator=(const std::string &s) {
        clear();
        append(s);
    }

    inline void cord_buf::swap(cord_buf &other) {
        const SmallView tmp = other._sv;
        other._sv = _sv;
        _sv = tmp;
    }

    inline int cord_buf::cut_until(cord_buf *out, char const *delim) {
        if (*delim) {
            if (!*(delim + 1)) {
                return _cut_by_char(out, *delim);
            } else {
                return _cut_by_delim(out, delim, strlen(delim));
            }
        }
        return -1;
    }

    inline int cord_buf::cut_until(cord_buf *out, const std::string &delim) {
        if (delim.length() == 1UL) {
            return _cut_by_char(out, delim[0]);
        } else if (delim.length() > 1UL) {
            return _cut_by_delim(out, delim.data(), delim.length());
        } else {
            return -1;
        }
    }

    inline int cord_buf::append(const std::string &s) {
        return append(s.data(), s.length());
    }

    inline std::string cord_buf::to_string() const {
        std::string s;
        copy_to(&s);
        return s;
    }

    inline bool cord_buf::empty() const {
        return _small() ? !_sv.refs[0].block : !_bv.nbytes;
    }

    inline size_t cord_buf::length() const {
        return _small() ?
               (_sv.refs[0].length + _sv.refs[1].length) : _bv.nbytes;
    }

    inline bool cord_buf::_small() const {
        return _bv.magic >= 0;
    }

    inline size_t cord_buf::_ref_num() const {
        return _small()
               ? (!!_sv.refs[0].block + !!_sv.refs[1].block) : _bv.nref;
    }

    inline cord_buf::BlockRef &cord_buf::_front_ref() {
        return _small() ? _sv.refs[0] : _bv.refs[_bv.start];
    }

    inline const cord_buf::BlockRef &cord_buf::_front_ref() const {
        return _small() ? _sv.refs[0] : _bv.refs[_bv.start];
    }

    inline cord_buf::BlockRef &cord_buf::_back_ref() {
        return _small() ? _sv.refs[!!_sv.refs[1].block] : _bv.ref_at(_bv.nref - 1);
    }

    inline const cord_buf::BlockRef &cord_buf::_back_ref() const {
        return _small() ? _sv.refs[!!_sv.refs[1].block] : _bv.ref_at(_bv.nref - 1);
    }

    inline cord_buf::BlockRef &cord_buf::_ref_at(size_t i) {
        return _small() ? _sv.refs[i] : _bv.ref_at(i);
    }

    inline const cord_buf::BlockRef &cord_buf::_ref_at(size_t i) const {
        return _small() ? _sv.refs[i] : _bv.ref_at(i);
    }

    inline const cord_buf::BlockRef *cord_buf::_pref_at(size_t i) const {
        if (_small()) {
            return i < (size_t) (!!_sv.refs[0].block + !!_sv.refs[1].block) ? &_sv.refs[i] : NULL;
        } else {
            return i < _bv.nref ? &_bv.ref_at(i) : NULL;
        }
    }

    inline bool operator==(const cord_buf::BlockRef &r1, const cord_buf::BlockRef &r2) {
        return r1.offset == r2.offset && r1.length == r2.length &&
               r1.block == r2.block;
    }

    inline bool operator!=(const cord_buf::BlockRef &r1, const cord_buf::BlockRef &r2) {
        return !(r1 == r2);
    }

    inline void cord_buf::_push_back_ref(const BlockRef &r) {
        if (_small()) {
            return _push_or_move_back_ref_to_smallview<false>(r);
        } else {
            return _push_or_move_back_ref_to_bigview<false>(r);
        }
    }

    inline void cord_buf::_move_back_ref(const BlockRef &r) {
        if (_small()) {
            return _push_or_move_back_ref_to_smallview<true>(r);
        } else {
            return _push_or_move_back_ref_to_bigview<true>(r);
        }
    }

////////////////  cord_buf_cutter ////////////////
    inline size_t cord_buf_cutter::remaining_bytes() const {
        if (_block) {
            return (char *) _data_end - (char *) _data + _buf->size() - _buf->_front_ref().length;
        } else {
            return _buf->size();
        }
    }

    inline bool cord_buf_cutter::cut1(void *c) {
        if (_data == _data_end) {
            if (!load_next_ref()) {
                return false;
            }
        }
        *(char *) c = *(const char *) _data;
        _data = (char *) _data + 1;
        return true;
    }

    inline const void *cord_buf_cutter::fetch1() {
        if (_data == _data_end) {
            if (!load_next_ref()) {
                return NULL;
            }
        }
        return _data;
    }

    inline size_t cord_buf_cutter::copy_to(void *out, size_t n) {
        size_t size = (char *) _data_end - (char *) _data;
        if (n <= size) {
            memcpy(out, _data, n);
            return n;
        }
        return slower_copy_to(out, n);
    }

    inline size_t cord_buf_cutter::pop_front(size_t n) {
        const size_t saved_n = n;
        do {
            const size_t size = (char *) _data_end - (char *) _data;
            if (n <= size) {
                _data = (char *) _data + n;
                return saved_n;
            }
            n -= size;
            if (!load_next_ref()) {
                return saved_n - n;
            }
        } while (true);
    }

    inline size_t cord_buf_cutter::cutn(std::string *out, size_t n) {
        if (n == 0) {
            return 0;
        }
        const size_t len = remaining_bytes();
        if (n > len) {
            n = len;
        }
        const size_t old_size = out->size();
        out->resize(out->size() + n);
        return cutn(&(*out)[old_size], n);
    }

    inline cord_buf_bytes_iterator::cord_buf_bytes_iterator(const melon::cord_buf &buf)
            : _block_begin(NULL), _block_end(NULL), _block_count(0),
              _bytes_left(buf.length()), _buf(&buf) {
        try_next_block();
    }

    inline cord_buf_bytes_iterator::cord_buf_bytes_iterator(const cord_buf_bytes_iterator &it)
            : _block_begin(it._block_begin), _block_end(it._block_end), _block_count(it._block_count),
              _bytes_left(it._bytes_left), _buf(it._buf) {
    }

    inline cord_buf_bytes_iterator::cord_buf_bytes_iterator(
            const cord_buf_bytes_iterator &it, size_t bytes_left)
            : _block_begin(it._block_begin), _block_end(it._block_end), _block_count(it._block_count),
              _bytes_left(bytes_left), _buf(it._buf) {
        //MELON_CHECK_LE(_bytes_left, it._bytes_left);
        if (_block_end > _block_begin + _bytes_left) {
            _block_end = _block_begin + _bytes_left;
        }
    }

    inline void cord_buf_bytes_iterator::try_next_block() {
        if (_bytes_left == 0) {
            return;
        }
        std::string_view s = _buf->backing_block(_block_count++);
        _block_begin = s.data();
        _block_end = s.data() + std::min(s.size(), (size_t) _bytes_left);
    }

    inline void cord_buf_bytes_iterator::operator++() {
        ++_block_begin;
        --_bytes_left;
        if (_block_begin == _block_end) {
            try_next_block();
        }
    }

    inline size_t cord_buf_bytes_iterator::copy_and_forward(void *buf, size_t n) {
        size_t nc = 0;
        while (nc < n && _bytes_left != 0) {
            const size_t block_size = _block_end - _block_begin;
            const size_t to_copy = std::min(block_size, n - nc);
            memcpy((char *) buf + nc, _block_begin, to_copy);
            _block_begin += to_copy;
            _bytes_left -= to_copy;
            nc += to_copy;
            if (_block_begin == _block_end) {
                try_next_block();
            }
        }
        return nc;
    }

    inline size_t cord_buf_bytes_iterator::copy_and_forward(std::string *s, size_t n) {
        bool resized = false;
        if (s->size() < n) {
            resized = true;
            s->resize(n);
        }
        const size_t nc = copy_and_forward(const_cast<char *>(s->data()), n);
        if (nc < n && resized) {
            s->resize(nc);
        }
        return nc;
    }

    inline size_t cord_buf_bytes_iterator::forward(size_t n) {
        size_t nc = 0;
        while (nc < n && _bytes_left != 0) {
            const size_t block_size = _block_end - _block_begin;
            const size_t to_copy = std::min(block_size, n - nc);
            _block_begin += to_copy;
            _bytes_left -= to_copy;
            nc += to_copy;
            if (_block_begin == _block_end) {
                try_next_block();
            }
        }
        return nc;
    }

}  // namespace melon

#endif  // MELON_IO_CORD_BUF_INL_H_
