
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_IO_CORD_BUF_H_
#define MELON_IO_CORD_BUF_H_

#include <sys/uio.h>                             // iovec
#include <stdint.h>                              // uint32_t
#include <string>                                // std::string
#include <ostream>                               // std::ostream
#include <string_view>
#include <utility>
#include "melon/io/reader_writer.h"
#include "melon/io/binary_printer.h"

// For cord_buf::appendv(const const_iovec*, size_t). The only difference of this
// struct from iovec (defined in sys/uio.h) is that iov_base is `const void*'
// which is assignable by const pointers w/o any error.
extern "C" {
struct const_iovec {
    const void *iov_base;
    size_t iov_len;
};
struct ssl_st;
}

namespace melon {

    // cord_buf is a non-continuous buffer that can be cut and combined w/o copying
    // payload. It can be read from or flushed into file descriptors as well.
    // cord_buf is [thread-compatible]. Namely using different cord_buf in different
    // threads simultaneously is safe, and reading a static cord_buf from different
    // threads is safe as well.
    // cord_buf is [NOT thread-safe]. Modifying a same cord_buf from different threads
    // simultaneously is unsafe and likely to crash.
    class cord_buf {
        friend class cord_buf_as_zero_copy_input_stream;

        friend class cord_buf_as_zero_copy_output_stream;

        friend class cord_buf_bytes_iterator;

        friend class cord_buf_cutter;

    public:
        static const size_t DEFAULT_BLOCK_SIZE = 8192;
        static const size_t INITIAL_CAP = 32; // must be power of 2

        struct buf_block;

        // can't directly use `struct iovec' here because we also need to access the
        // reference counter(nshared) in Block*
        struct BlockRef {
            // NOTICE: first bit of `offset' is shared with BigView::start
            uint32_t offset;
            uint32_t length;
            buf_block *block;
        };

        // cord_buf is essentially a tiny queue of BlockRefs.
        struct SmallView {
            BlockRef refs[2];
        };

        struct BigView {
            int32_t magic;
            uint32_t start;
            BlockRef *refs;
            uint32_t nref;
            uint32_t cap_mask;
            size_t nbytes;

            const BlockRef &ref_at(uint32_t i) const { return refs[(start + i) & cap_mask]; }

            BlockRef &ref_at(uint32_t i) { return refs[(start + i) & cap_mask]; }

            uint32_t capacity() const { return cap_mask + 1; }
        };

        struct Movable {
            explicit Movable(cord_buf &v) : _v(&v) {}

            cord_buf &value() const { return *_v; }

        private:
            cord_buf *_v;
        };

        typedef uint64_t Area;
        static const Area INVALID_AREA = 0;

        cord_buf();

        cord_buf(const cord_buf &);

        cord_buf(const Movable &);

        ~cord_buf() { clear(); }

        void operator=(const cord_buf &);

        void operator=(const Movable &);

        void operator=(const char *);

        void operator=(const std::string &);

        // Exchange internal fields with another cord_buf.
        void swap(cord_buf &);

        // Pop n bytes from front side
        // If n == 0, nothing popped; if n >= length(), all bytes are popped
        // Returns bytes popped.
        size_t pop_front(size_t n);

        // Pop n bytes from back side
        // If n == 0, nothing popped; if n >= length(), all bytes are popped
        // Returns bytes popped.
        size_t pop_back(size_t n);

        // Cut off n bytes from front side and APPEND to `out'
        // If n == 0, nothing cut; if n >= length(), all bytes are cut
        // Returns bytes cut.
        size_t cutn(cord_buf *out, size_t n);

        size_t cutn(void *out, size_t n);

        size_t cutn(std::string *out, size_t n);

        // Cut off 1 byte from the front side and set to *c
        // Return true on cut, false otherwise.
        bool cut1(void *c);

        // Cut from front side until the characters matches `delim', append
        // data before the matched characters to `out'.
        // Returns 0 on success, -1 when there's no match (including empty `delim')
        // or other errors.
        int cut_until(cord_buf *out, char const *delim);

        // std::string version, `delim' could be binary
        int cut_until(cord_buf *out, const std::string &delim);

        // Cut at most `size_hint' bytes(approximately) into the writer
        // Returns bytes cut on success, -1 otherwise and errno is set.
        ssize_t cut_into_writer(base_writer *writer, size_t size_hint = 1024 * 1024);

        // Cut at most `size_hint' bytes(approximately) into the file descriptor
        // Returns bytes cut on success, -1 otherwise and errno is set.
        ssize_t cut_into_file_descriptor(int fd, size_t size_hint = 1024 * 1024);

        // Cut at most `size_hint' bytes(approximately) into the file descriptor at
        // a given offset(from the start of the file). The file offset is not changed.
        // If `offset' is negative, does exactly what cut_into_file_descriptor does.
        // Returns bytes cut on success, -1 otherwise and errno is set.
        //
        // NOTE: POSIX requires that a file open with the O_APPEND flag should
        // not affect pwrite(). However, on Linux, if |fd| is open with O_APPEND,
        // pwrite() appends data to the end of the file, regardless of the value
        // of |offset|.
        ssize_t pcut_into_file_descriptor(int fd, off_t offset /*NOTE*/,
                                          size_t size_hint = 1024 * 1024);

        // Cut `count' number of `pieces' into the writer.
        // Returns bytes cut on success, -1 otherwise and errno is set.
        static ssize_t cut_multiple_into_writer(
                base_writer *writer, cord_buf *const *pieces, size_t count);

        // Cut `count' number of `pieces' into the file descriptor.
        // Returns bytes cut on success, -1 otherwise and errno is set.
        static ssize_t cut_multiple_into_file_descriptor(
                int fd, cord_buf *const *pieces, size_t count);

        // Cut `count' number of `pieces' into file descriptor `fd' at a given
        // offset. The file offset is not changed.
        // If `offset' is negative, does exactly what cut_multiple_into_file_descriptor
        // does.
        // Read NOTE of pcut_into_file_descriptor.
        // Returns bytes cut on success, -1 otherwise and errno is set.
        static ssize_t pcut_multiple_into_file_descriptor(
                int fd, off_t offset, cord_buf *const *pieces, size_t count);


        // Append another cord_buf to back side, payload of the cord_buf is shared
        // rather than copied.
        void append(const cord_buf &other);

        // Append content of `other' to self and clear `other'.
        void append(const Movable &other);

        // ===================================================================
        // Following push_back()/append() are just implemented for convenience
        // and occasional usages, they're relatively slow because of the overhead
        // of frequent BlockRef-management and reference-countings. If you get
        // a lot of push_back/append to do, you should use cord_buf_appender or
        // cord_buf_builder instead, which reduce overhead by owning cord_buf::Block.
        // ===================================================================

        // Append a character to back side. (with copying)
        // Returns 0 on success, -1 otherwise.
        int push_back(char c);

        // Append `data' with `count' bytes to back side. (with copying)
        // Returns 0 on success(include count == 0), -1 otherwise.
        int append(void const *data, size_t count);

        // Append multiple data to back side in one call, faster than appending
        // one by one separately.
        // Returns 0 on success, -1 otherwise.
        // Example:
        //   const_iovec vec[] = { { data1, len1 },
        //                         { data2, len2 },
        //                         { data3, len3 } };
        //   foo.appendv(vec, arraysize(vec));
        int appendv(const const_iovec vec[], size_t n);

        int appendv(const iovec *vec, size_t n) { return appendv((const const_iovec *) vec, n); }

        // Append a c-style string to back side. (with copying)
        // Returns 0 on success, -1 otherwise.
        // NOTE: Returns 0 when `s' is empty.
        int append(char const *s);

        // Append a std::string to back side. (with copying)
        // Returns 0 on success, -1 otherwise.
        // NOTE: Returns 0 when `s' is empty.
        int append(const std::string &s);

        // Append the user-data to back side WITHOUT copying.
        // The user-data can be split and shared by smaller cord_bufs and will be
        // deleted using the deleter func when no cord_buf references it anymore.
        int append_user_data(void *data, size_t size, void (*deleter)(void *));

        // Resizes the buf to a length of n characters.
        // If n is smaller than the current length, all bytes after n will be
        // truncated.
        // If n is greater than the current length, the buffer would be append with
        // as many |c| as needed to reach a size of n. If c is not specified,
        // null-character would be appended.
        // Returns 0 on success, -1 otherwise.
        int resize(size_t n) { return resize(n, '\0'); }

        int resize(size_t n, char c);

        // Reserve `n' uninitialized bytes at back-side.
        // Returns an object representing the reserved area, INVALID_AREA on failure.
        // NOTE: reserve(0) returns INVALID_AREA.
        Area reserve(size_t n);

        // [EXTREMELY UNSAFE]
        // Copy `data' to the reserved `area'. `data' must be as long as the
        // reserved size.
        // Returns 0 on success, -1 otherwise.
        // [Rules]
        // 1. Make sure the cord_buf to be assigned was NOT cut/pop from front side
        //    after reserving, otherwise behavior of this function is undefined,
        //    even if it returns 0.
        // 2. Make sure the cord_buf to be assigned was NOT copied to/from another
        //    cord_buf after reserving to prevent underlying blocks from being shared,
        //    otherwise the assignment affects all cord_buf sharing the blocks, which
        //    is probably not what we want.
        int unsafe_assign(Area area, const void *data);

        // Append min(n, length()) bytes starting from `pos' at front side to `buf'.
        // The real payload is shared rather than copied.
        // Returns bytes copied.
        size_t append_to(cord_buf *buf, size_t n = (size_t) -1L, size_t pos = 0) const;

        // Explicitly declare this overload as error to avoid copy_to(melon::cord_buf*)
        // from being interpreted as copy_to(void*) by the compiler (which causes
        // undefined behavior).
        size_t copy_to(cord_buf *buf, size_t n = (size_t) -1L, size_t pos = 0) const
        // the error attribute in not available in gcc 3.4
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
        __attribute__ (( error("Call append_to(cord_buf*) instead") ))
#endif
        ;

        // Copy min(n, length()) bytes starting from `pos' at front side into `buf'.
        // Returns bytes copied.
        size_t copy_to(void *buf, size_t n = (size_t) -1L, size_t pos = 0) const;

        // NOTE: first parameter is not std::string& because user may passes
        // a pointer of std::string by mistake, in which case, compiler would
        // call the void* version which crashes definitely.
        size_t copy_to(std::string *s, size_t n = (size_t) -1L, size_t pos = 0) const;

        size_t append_to(std::string *s, size_t n = (size_t) -1L, size_t pos = 0) const;

        // Copy min(n, length()) bytes staring from `pos' at front side into
        // `cstr' and end it with '\0'.
        // `cstr' must be as long as min(n, length())+1.
        // Returns bytes copied (not including ending '\0')
        size_t copy_to_cstr(char *cstr, size_t n = (size_t) -1L, size_t pos = 0) const;

        // Convert all data in this buffer to a std::string.
        std::string to_string() const;

        // Get `n' front-side bytes with minimum copying. Length of `aux_buffer'
        // must not be less than `n'.
        // Returns:
        //   NULL            -  n is greater than length()
        //   aux_buffer      -  n bytes are copied into aux_buffer
        //   internal buffer -  the bytes are stored continuously in the internal
        //                      buffer, no copying is needed. This function does not
        //                      add additional reference to the underlying block,
        //                      so user should not change this cord_buf during using
        //                      the internal buffer.
        // If n == 0 and buffer is empty, return value is undefined.
        const void *fetch(void *aux_buffer, size_t n) const;

        // Fetch one character from front side.
        // Returns pointer to the character, NULL on empty.
        const void *fetch1() const;

        // Remove all data
        void clear();

        // True iff there's no data
        bool empty() const;

        // Number of bytes
        size_t length() const;

        size_t size() const { return length(); }

        // Get number of Blocks in use. block_memory = block_count * BLOCK_SIZE
        static size_t block_count();

        static size_t block_memory();

        static size_t new_bigview_count();

        static size_t block_count_hit_tls_threshold();

        // Equal with a string/cord_buf or not.
        bool equals(const std::string_view &) const;

        bool equals(const cord_buf &other) const;

        // Get the number of backing blocks
        size_t backing_block_num() const { return _ref_num(); }

        // Get #i backing_block, an empty std::string_view is returned if no such block
        std::string_view backing_block(size_t i) const;

        // Make a movable version of self
        Movable movable() { return Movable(*this); }

    protected:
        int _cut_by_char(cord_buf *out, char);

        int _cut_by_delim(cord_buf *out, char const *dbegin, size_t ndelim);

        // Returns: true iff this should be viewed as SmallView
        bool _small() const;

        template<bool MOVE>
        void _push_or_move_back_ref_to_smallview(const BlockRef &);

        template<bool MOVE>
        void _push_or_move_back_ref_to_bigview(const BlockRef &);

        // Push a BlockRef to back side
        // NOTICE: All fields of the ref must be initialized or assigned
        //         properly, or it will ruin this queue
        void _push_back_ref(const BlockRef &);

        // Move a BlockRef to back side. After calling this function, content of
        // the BlockRef will be invalid and should never be used again.
        void _move_back_ref(const BlockRef &);

        // Pop a BlockRef from front side.
        // Returns: 0 on success and -1 on empty.
        int _pop_front_ref() { return _pop_or_moveout_front_ref<false>(); }

        // Move a BlockRef out from front side.
        // Returns: 0 on success and -1 on empty.
        int _moveout_front_ref() { return _pop_or_moveout_front_ref<true>(); }

        template<bool MOVEOUT>
        int _pop_or_moveout_front_ref();

        // Pop a BlockRef from back side.
        // Returns: 0 on success and -1 on empty.
        int _pop_back_ref();

        // Number of refs in the queue
        size_t _ref_num() const;

        // Get reference to front/back BlockRef in the queue
        // should not be called if queue is empty or the behavior is undefined
        BlockRef &_front_ref();

        const BlockRef &_front_ref() const;

        BlockRef &_back_ref();

        const BlockRef &_back_ref() const;

        // Get reference to n-th BlockRef(counting from front) in the queue
        // NOTICE: should not be called if queue is empty and the `n' must
        //         be inside [0, _ref_num()-1] or behavior is undefined
        BlockRef &_ref_at(size_t i);

        const BlockRef &_ref_at(size_t i) const;

        // Get pointer to n-th BlockRef(counting from front)
        // If i is out-of-range, NULL is returned.
        const BlockRef *_pref_at(size_t i) const;

    private:
        union {
            BigView _bv;
            SmallView _sv;
        };
    };

    std::ostream &operator<<(std::ostream &, const cord_buf &buf);

    inline bool operator==(const melon::cord_buf &b, const std::string_view &s) { return b.equals(s); }

    inline bool operator==(const std::string_view &s, const melon::cord_buf &b) { return b.equals(s); }

    inline bool operator!=(const melon::cord_buf &b, const std::string_view &s) { return !b.equals(s); }

    inline bool operator!=(const std::string_view &s, const melon::cord_buf &b) { return !b.equals(s); }

    inline bool operator==(const melon::cord_buf &b1, const melon::cord_buf &b2) { return b1.equals(b2); }

    inline bool operator!=(const melon::cord_buf &b1, const melon::cord_buf &b2) { return !b1.equals(b2); }

    // IOPortal is a subclass of cord_buf that can read from file descriptors.
    // Typically used as the buffer to store bytes from sockets.
    class IOPortal : public cord_buf {
    public:
        IOPortal() : _block(NULL) {}

        IOPortal(const IOPortal &rhs) : cord_buf(rhs), _block(NULL) {}

        ~IOPortal();

        IOPortal &operator=(const IOPortal &rhs);

        // Read at most `max_count' bytes from the reader and append to self.
        ssize_t append_from_reader(base_reader *reader, size_t max_count);

        // Read at most `max_count' bytes from file descriptor `fd' and
        // append to self.
        ssize_t append_from_file_descriptor(int fd, size_t max_count);

        // Read at most `max_count' bytes from file descriptor `fd' at a given
        // offset and append to self. The file offset is not changed.
        // If `offset' is negative, does exactly what append_from_file_descriptor does.
        ssize_t pappend_from_file_descriptor(int fd, off_t offset, size_t max_count);


        // Remove all data inside and return cached blocks.
        void clear();

        // Return cached blocks to TLS. This function should be called by users
        // when this IOPortal are cut into intact messages and becomes empty, to
        // let continuing code on cord_buf to reuse the blocks. Calling this function
        // after each call to append_xxx does not make sense and may hurt
        // performance. Read comments on field `_block' below.
        void return_cached_blocks();

    private:
        static void return_cached_blocks_impl(buf_block *);

        // Cached blocks for appending. Notice that the blocks are released
        // until return_cached_blocks()/clear()/dtor() are called, rather than
        // released after each append_xxx(), which makes messages read from one
        // file descriptor more likely to share blocks and have less BlockRefs.
        buf_block *_block;
    };

    // Specialized utility to cut from cord_buf faster than using corresponding
    // methods in cord_buf.
    // Designed for efficiently parsing data from cord_buf.
    // The cut cord_buf can be appended during cutting.
    class cord_buf_cutter {
    public:
        explicit cord_buf_cutter(melon::cord_buf *buf);

        ~cord_buf_cutter();

        // Cut off n bytes and APPEND to `out'
        // Returns bytes cut.
        size_t cutn(melon::cord_buf *out, size_t n);

        size_t cutn(std::string *out, size_t n);

        size_t cutn(void *out, size_t n);

        // Cut off 1 byte from the front side and set to *c
        // Return true on cut, false otherwise.
        bool cut1(void *data);

        // Copy n bytes into `data'
        // Returns bytes copied.
        size_t copy_to(void *data, size_t n);

        // Fetch one character.
        // Returns pointer to the character, NULL on empty
        const void *fetch1();

        // Pop n bytes from front side
        // Returns bytes popped.
        size_t pop_front(size_t n);

        // Uncut bytes
        size_t remaining_bytes() const;

    private:
        size_t slower_copy_to(void *data, size_t n);

        bool load_next_ref();

    private:
        void *_data;
        void *_data_end;
        cord_buf::buf_block *_block;
        cord_buf *_buf;
    };


    // Iterate bytes of a cord_buf.
    // During iteration, the iobuf should NOT be changed.
    class cord_buf_bytes_iterator {
    public:
        explicit cord_buf_bytes_iterator(const melon::cord_buf &buf);

        // Construct from another iterator.
        cord_buf_bytes_iterator(const cord_buf_bytes_iterator &it);

        cord_buf_bytes_iterator(const cord_buf_bytes_iterator &it, size_t bytes_left);

        // Returning unsigned is safer than char which would be more error prone
        // to bitwise operations. For example: in "uint32_t value = *it", value
        // is (unexpected) 4294967168 when *it returns (char)128.
        unsigned char operator*() const { return (unsigned char) *_block_begin; }

        operator const void *() const { return (const void *) !!_bytes_left; }

        void operator++();

        void operator++(int) { return operator++(); }

        // Copy at most n bytes into buf, forwarding this iterator.
        // Returns bytes copied.
        size_t copy_and_forward(void *buf, size_t n);

        size_t copy_and_forward(std::string *s, size_t n);

        // Just forward this iterator for at most n bytes.
        size_t forward(size_t n);

        // Append at most n bytes into buf, forwarding this iterator. Data are
        // referenced rather than copied.
        size_t append_and_forward(melon::cord_buf *buf, size_t n);

        bool forward_one_block(const void **data, size_t *size);

        size_t bytes_left() const { return _bytes_left; }

    private:
        void try_next_block();

        const char *_block_begin;
        const char *_block_end;
        uint32_t _block_count;
        uint32_t _bytes_left;
        const melon::cord_buf *_buf;
    };

}  // namespace melon

namespace std {
    template<>
    inline void swap(melon::cord_buf &a, melon::cord_buf &b) {
        return a.swap(b);
    }
} // namespace std

#include "melon/io/cord_buf_inl.h"

#endif  // MELON_IO_CORD_BUF_H_
