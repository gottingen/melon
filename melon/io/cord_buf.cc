
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include <sys/syscall.h>                   // syscall
#include <fcntl.h>                         // O_RDONLY
#include <errno.h>                         // errno
#include <limits.h>                        // CHAR_BIT
#include <stdexcept>                       // std::invalid_argument
#include "melon/base/static_atomic.h"                // std::atomic
#include "melon/thread/thread.h"             // thread_atexit
#include "melon/log/logging.h"                  // MELON_CHECK, MELON_LOG
#include "melon/base/fd_guard.h"                 // melon::base::fd_guard
#include "melon/io/cord_buf.h"
#include "melon/base/profile.h"

namespace melon {

    namespace io_internal {

        typedef ssize_t (*iov_function)(int fd, const struct iovec *vector,
                                        int count, off_t offset);

        // Userpsace preadv
        static ssize_t user_preadv(int fd, const struct iovec *vector,
                                   int count, off_t offset) {
            ssize_t total_read = 0;
            for (int i = 0; i < count; ++i) {
                const ssize_t rc = ::pread(fd, vector[i].iov_base, vector[i].iov_len, offset);
                if (rc <= 0) {
                    return total_read > 0 ? total_read : rc;
                }
                total_read += rc;
                offset += rc;
                if (rc < (ssize_t) vector[i].iov_len) {
                    break;
                }
            }
            return total_read;
        }

        static ssize_t user_pwritev(int fd, const struct iovec *vector,
                                    int count, off_t offset) {
            ssize_t total_write = 0;
            for (int i = 0; i < count; ++i) {
                const ssize_t rc = ::pwrite(fd, vector[i].iov_base, vector[i].iov_len, offset);
                if (rc <= 0) {
                    return total_write > 0 ? total_write : rc;
                }
                total_write += rc;
                offset += rc;
                if (rc < (ssize_t) vector[i].iov_len) {
                    break;
                }
            }
            return total_write;
        }

#if defined(MELON_PROCESSOR_X86_64)

#ifndef SYS_preadv
#define SYS_preadv 295
#endif  // SYS_preadv

#ifndef SYS_pwritev
#define SYS_pwritev 296
#endif // SYS_pwritev

        // SYS_preadv/SYS_pwritev is available since Linux 2.6.30
        static ssize_t sys_preadv(int fd, const struct iovec *vector,
                                  int count, off_t offset) {
            return syscall(SYS_preadv, fd, vector, count, offset);
        }

        static ssize_t sys_pwritev(int fd, const struct iovec *vector,
                                   int count, off_t offset) {
            return syscall(SYS_pwritev, fd, vector, count, offset);
        }

        inline iov_function get_preadv_func() {
            melon::base::fd_guard fd(open("/dev/zero", O_RDONLY));
            if (fd < 0) {
                MELON_PLOG(WARNING) << "Fail to open /dev/zero";
                return user_preadv;
            }
#if defined(MELON_PLATFORM_OSX)
            return user_preadv;
#endif
            char dummy[1];
            iovec vec = {dummy, sizeof(dummy)};
            const int rc = syscall(SYS_preadv, (int) fd, &vec, 1, 0);
            if (rc < 0) {
                MELON_PLOG(WARNING) << "The kernel doesn't support SYS_preadv, "
                                       " use user_preadv instead";
                return user_preadv;
            }
            return sys_preadv;
        }

        inline iov_function get_pwritev_func() {
            melon::base::fd_guard fd(open("/dev/null", O_WRONLY));
            if (fd < 0) {
                MELON_PLOG(ERROR) << "Fail to open /dev/null";
                return user_pwritev;
            }
#if defined(MELON_PLATFORM_OSX)
            return user_pwritev;
#endif
            char dummy[1];
            iovec vec = {dummy, sizeof(dummy)};
            const int rc = syscall(SYS_pwritev, (int) fd, &vec, 1, 0);
            if (rc < 0) {
                MELON_PLOG(WARNING) << "The kernel doesn't support SYS_pwritev, "
                                       " use user_pwritev instead";
                return user_pwritev;
            }
            return sys_pwritev;
        }

#else   // MELON_PROCESSOR_X86_64

#warning "We don't check whether the kernel supports SYS_preadv or SYS_pwritev " \
         "when the arch is not X86_64, use user space preadv/pwritev directly"

        inline iov_function get_preadv_func() {
            return user_preadv;
        }

        inline iov_function get_pwritev_func() {
            return user_pwritev;
        }

#endif  // MELON_PROCESSOR_X86_64

        inline void *cp(void *__restrict dest, const void *__restrict src, size_t n) {
            // memcpy in gcc 4.8 seems to be faster enough.
            return memcpy(dest, src, n);
        }

        // Function pointers to allocate or deallocate memory for a cord_buf::Block
        void *(*blockmem_allocate)(size_t) = ::malloc;

        void (*blockmem_deallocate)(void *) = ::free;

        // Use default function pointers
        void reset_blockmem_allocate_and_deallocate() {
            blockmem_allocate = ::malloc;
            blockmem_deallocate = ::free;
        }

        melon::static_atomic<size_t> g_nblock = MELON_STATIC_ATOMIC_INIT(0);
        melon::static_atomic<size_t> g_blockmem = MELON_STATIC_ATOMIC_INIT(0);
        melon::static_atomic<size_t> g_newbigview = MELON_STATIC_ATOMIC_INIT(0);

    }  // namespace io_internal

    size_t cord_buf::block_count() {
        return io_internal::g_nblock.load(std::memory_order_relaxed);
    }

    size_t cord_buf::block_memory() {
        return io_internal::g_blockmem.load(std::memory_order_relaxed);
    }

    size_t cord_buf::new_bigview_count() {
        return io_internal::g_newbigview.load(std::memory_order_relaxed);
    }

    const uint16_t CORD_BUF_BLOCK_FLAGS_USER_DATA = 0x1;

    typedef void (*UserDataDeleter)(void *);

    struct UserDataExtension {
        UserDataDeleter deleter;
    };

    struct cord_buf::buf_block {
        std::atomic<int> nshared;
        uint16_t flags;
        uint16_t abi_check;  // original cap, never be zero.
        uint32_t size;
        uint32_t cap;
        buf_block *portal_next;
        // When flag is 0, data points to `size` bytes starting at `(char*)this+sizeof(Block)'
        // When flag & CORD_BUF_BLOCK_FLAGS_USER_DATA is non-0, data points to the user data and
        // the deleter is put in UserDataExtension at `(char*)this+sizeof(Block)'
        char *data;

        buf_block(char *data_in, uint32_t data_size)
                : nshared(1), flags(0), abi_check(0), size(0), cap(data_size), portal_next(nullptr), data(data_in) {
            io_internal::g_nblock.fetch_add(1, std::memory_order_relaxed);
            io_internal::g_blockmem.fetch_add(data_size + sizeof(buf_block),
                                        std::memory_order_relaxed);
        }

        buf_block(char *data_in, uint32_t data_size, UserDataDeleter deleter)
                : nshared(1), flags(CORD_BUF_BLOCK_FLAGS_USER_DATA), abi_check(0), size(data_size), cap(data_size),
                  portal_next(nullptr), data(data_in) {
            get_user_data_extension()->deleter = deleter;
        }

        // Undefined behavior when (flags & CORD_BUF_BLOCK_FLAGS_USER_DATA) is 0.
        UserDataExtension *get_user_data_extension() {
            char *p = (char *) this;
            return (UserDataExtension *) (p + sizeof(buf_block));
        }

        inline void check_abi() {
#ifndef NDEBUG
            if (abi_check != 0) {
                MELON_LOG(FATAL) << "Your program seems to wrongly contain two "
                                    "ABI-incompatible implementations of cord_buf";
            }
#endif
        }

        void inc_ref() {
            check_abi();
            nshared.fetch_add(1, std::memory_order_relaxed);
        }

        void dec_ref() {
            check_abi();
            if (nshared.fetch_sub(1, std::memory_order_release) == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                if (!flags) {
                    io_internal::g_nblock.fetch_sub(1, std::memory_order_relaxed);
                    io_internal::g_blockmem.fetch_sub(cap + sizeof(buf_block),
                                                std::memory_order_relaxed);
                    this->~buf_block();
                    io_internal::blockmem_deallocate(this);
                } else if (flags & CORD_BUF_BLOCK_FLAGS_USER_DATA) {
                    get_user_data_extension()->deleter(data);
                    this->~buf_block();
                    free(this);
                }
            }
        }

        int ref_count() const {
            return nshared.load(std::memory_order_relaxed);
        }

        bool full() const { return size >= cap; }

        size_t left_space() const { return cap - size; }
    };

    namespace io_internal {

        // for unit test
        int block_shared_count(cord_buf::buf_block const *b) { return b->ref_count(); }

        cord_buf::buf_block *get_portal_next(cord_buf::buf_block const *b) {
            return b->portal_next;
        }

        uint32_t block_cap(cord_buf::buf_block const *b) {
            return b->cap;
        }

        uint32_t block_size(cord_buf::buf_block const *b) {
            return b->size;
        }

        inline cord_buf::buf_block *create_block(const size_t block_size) {
            if (block_size > 0xFFFFFFFFULL) {
                MELON_LOG(FATAL) << "block_size=" << block_size << " is too large";
                return nullptr;
            }
            char *mem = (char *) io_internal::blockmem_allocate(block_size);
            if (mem == nullptr) {
                return nullptr;
            }
            return new(mem) cord_buf::buf_block(mem + sizeof(cord_buf::buf_block),
                                            block_size - sizeof(cord_buf::buf_block));
        }

        inline cord_buf::buf_block *create_block() {
            return create_block(cord_buf::DEFAULT_BLOCK_SIZE);
        }

        // === Share TLS blocks between appending operations ===
        // Max number of blocks in each TLS. This is a soft limit namely
        // release_tls_block_chain() may exceed this limit sometimes.
        const int MAX_BLOCKS_PER_THREAD = 8;

        struct TLSData {
            // Head of the TLS block chain.
            cord_buf::buf_block *block_head;

            // Number of TLS blocks
            int num_blocks;

            // True if the remote_tls_block_chain is registered to the thread.
            bool registered;
        };

        static __thread TLSData g_tls_data = {nullptr, 0, false};

        // Used in UT
        cord_buf::buf_block *get_tls_block_head() { return g_tls_data.block_head; }

        int get_tls_block_count() { return g_tls_data.num_blocks; }

        // Number of blocks that can't be returned to TLS which has too many block
        // already. This counter should be 0 in most scenarios, otherwise performance
        // of appending functions in IOPortal may be lowered.
        static melon::static_atomic<size_t> g_num_hit_tls_threshold = MELON_STATIC_ATOMIC_INIT(0);

        // Called in UT.
        void remove_tls_block_chain() {
            TLSData &tls_data = g_tls_data;
            cord_buf::buf_block *b = tls_data.block_head;
            if (!b) {
                return;
            }
            tls_data.block_head = nullptr;
            int n = 0;
            do {
                cord_buf::buf_block *const saved_next = b->portal_next;
                b->dec_ref();
                b = saved_next;
                ++n;
            } while (b);
            MELON_CHECK_EQ(n, tls_data.num_blocks);
            tls_data.num_blocks = 0;
        }

// Get a (non-full) block from TLS.
// Notice that the block is not removed from TLS.
        cord_buf::buf_block *share_tls_block() {
            TLSData &tls_data = g_tls_data;
            cord_buf::buf_block *const b = tls_data.block_head;
            if (b != nullptr && !b->full()) {
                return b;
            }
            cord_buf::buf_block *new_block = nullptr;
            if (b) {
                new_block = b;
                while (new_block && new_block->full()) {
                    cord_buf::buf_block *const saved_next = new_block->portal_next;
                    new_block->dec_ref();
                    --tls_data.num_blocks;
                    new_block = saved_next;
                }
            } else if (!tls_data.registered) {
                tls_data.registered = true;
                // Only register atexit at the first time
                melon::thread::atexit(remove_tls_block_chain);
            }
            if (!new_block) {
                new_block = create_block(); // may be nullptr
                if (new_block) {
                    ++tls_data.num_blocks;
                }
            }
            tls_data.block_head = new_block;
            return new_block;
        }

        // Return one block to TLS.
        inline void release_tls_block(cord_buf::buf_block *b) {
            if (!b) {
                return;
            }
            TLSData &tls_data = g_tls_data;
            if (b->full()) {
                b->dec_ref();
            } else if (tls_data.num_blocks >= MAX_BLOCKS_PER_THREAD) {
                b->dec_ref();
                g_num_hit_tls_threshold.fetch_add(1, std::memory_order_relaxed);
            } else {
                b->portal_next = tls_data.block_head;
                tls_data.block_head = b;
                ++tls_data.num_blocks;
                if (!tls_data.registered) {
                    tls_data.registered = true;
                    melon::thread::atexit(remove_tls_block_chain);
                }
            }
        }

        // Return chained blocks to TLS.
        // NOTE: b MUST be non-nullptr and all blocks linked SHOULD not be full.
        void release_tls_block_chain(cord_buf::buf_block *b) {
            TLSData &tls_data = g_tls_data;
            size_t n = 0;
            if (tls_data.num_blocks >= MAX_BLOCKS_PER_THREAD) {
                do {
                    ++n;
                    cord_buf::buf_block *const saved_next = b->portal_next;
                    b->dec_ref();
                    b = saved_next;
                } while (b);
                g_num_hit_tls_threshold.fetch_add(n, std::memory_order_relaxed);
                return;
            }
            cord_buf::buf_block *first_b = b;
            cord_buf::buf_block *last_b = nullptr;
            do {
                ++n;
                MELON_CHECK(!b->full());
                if (b->portal_next == nullptr) {
                    last_b = b;
                    break;
                }
                b = b->portal_next;
            } while (true);
            last_b->portal_next = tls_data.block_head;
            tls_data.block_head = first_b;
            tls_data.num_blocks += n;
            if (!tls_data.registered) {
                tls_data.registered = true;
                melon::thread::atexit(remove_tls_block_chain);
            }
        }

// Get and remove one (non-full) block from TLS. If TLS is empty, create one.
        cord_buf::buf_block *acquire_tls_block() {
            TLSData &tls_data = g_tls_data;
            cord_buf::buf_block *b = tls_data.block_head;
            if (!b) {
                return create_block();
            }
            while (b->full()) {
                cord_buf::buf_block *const saved_next = b->portal_next;
                b->dec_ref();
                tls_data.block_head = saved_next;
                --tls_data.num_blocks;
                b = saved_next;
                if (!b) {
                    return create_block();
                }
            }
            tls_data.block_head = b->portal_next;
            --tls_data.num_blocks;
            b->portal_next = nullptr;
            return b;
        }

        inline cord_buf::BlockRef *acquire_blockref_array(size_t cap) {
            io_internal::g_newbigview.fetch_add(1, std::memory_order_relaxed);
            return new cord_buf::BlockRef[cap];
        }

        inline cord_buf::BlockRef *acquire_blockref_array() {
            return acquire_blockref_array(cord_buf::INITIAL_CAP);
        }

        inline void release_blockref_array(cord_buf::BlockRef *refs, size_t cap) {
            delete[] refs;
        }

    }  // namespace io_internal

    size_t cord_buf::block_count_hit_tls_threshold() {
        return io_internal::g_num_hit_tls_threshold.load(std::memory_order_relaxed);
    }

    static_assert(sizeof(cord_buf::SmallView) == sizeof(cord_buf::BigView),
                  "sizeof_small_and_big_view_should_equal");

    static_assert(cord_buf::DEFAULT_BLOCK_SIZE / 4096 * 4096 == cord_buf::DEFAULT_BLOCK_SIZE,
                  "sizeof_block_should_be_multiply_of_4096");

    const cord_buf::Area cord_buf::INVALID_AREA;

    cord_buf::cord_buf(const cord_buf &rhs) {
        if (rhs._small()) {
            _sv = rhs._sv;
            if (_sv.refs[0].block) {
                _sv.refs[0].block->inc_ref();
            }
            if (_sv.refs[1].block) {
                _sv.refs[1].block->inc_ref();
            }
        } else {
            _bv.magic = -1;
            _bv.start = 0;
            _bv.nref = rhs._bv.nref;
            _bv.cap_mask = rhs._bv.cap_mask;
            _bv.nbytes = rhs._bv.nbytes;
            _bv.refs = io_internal::acquire_blockref_array(_bv.capacity());
            for (size_t i = 0; i < _bv.nref; ++i) {
                _bv.refs[i] = rhs._bv.ref_at(i);
                _bv.refs[i].block->inc_ref();
            }
        }
    }

    void cord_buf::operator=(const cord_buf &rhs) {
        if (this == &rhs) {
            return;
        }
        if (!rhs._small() && !_small() && _bv.cap_mask == rhs._bv.cap_mask) {
            // Reuse array of refs
            // Remove references to previous blocks.
            for (size_t i = 0; i < _bv.nref; ++i) {
                _bv.ref_at(i).block->dec_ref();
            }
            // References blocks in rhs.
            _bv.start = 0;
            _bv.nref = rhs._bv.nref;
            _bv.nbytes = rhs._bv.nbytes;
            for (size_t i = 0; i < _bv.nref; ++i) {
                _bv.refs[i] = rhs._bv.ref_at(i);
                _bv.refs[i].block->inc_ref();
            }
        } else {
            this->~cord_buf();
            new(this) cord_buf(rhs);
        }
    }

    template<bool MOVE>
    void cord_buf::_push_or_move_back_ref_to_smallview(const BlockRef &r) {
        BlockRef *const refs = _sv.refs;
        if (nullptr == refs[0].block) {
            refs[0] = r;
            if (!MOVE) {
                r.block->inc_ref();
            }
            return;
        }
        if (nullptr == refs[1].block) {
            if (refs[0].block == r.block &&
                refs[0].offset + refs[0].length == r.offset) { // Merge ref
                refs[0].length += r.length;
                if (MOVE) {
                    r.block->dec_ref();
                }
                return;
            }
            refs[1] = r;
            if (!MOVE) {
                r.block->inc_ref();
            }
            return;
        }
        if (refs[1].block == r.block &&
            refs[1].offset + refs[1].length == r.offset) { // Merge ref
            refs[1].length += r.length;
            if (MOVE) {
                r.block->dec_ref();
            }
            return;
        }
        // Convert to BigView
        BlockRef *new_refs = io_internal::acquire_blockref_array();
        new_refs[0] = refs[0];
        new_refs[1] = refs[1];
        new_refs[2] = r;
        const size_t new_nbytes = refs[0].length + refs[1].length + r.length;
        if (!MOVE) {
            r.block->inc_ref();
        }
        _bv.magic = -1;
        _bv.start = 0;
        _bv.refs = new_refs;
        _bv.nref = 3;
        _bv.cap_mask = INITIAL_CAP - 1;
        _bv.nbytes = new_nbytes;
    }

// Explicitly initialize templates.
    template void cord_buf::_push_or_move_back_ref_to_smallview<true>(const BlockRef &);

    template void cord_buf::_push_or_move_back_ref_to_smallview<false>(const BlockRef &);

    template<bool MOVE>
    void cord_buf::_push_or_move_back_ref_to_bigview(const BlockRef &r) {
        BlockRef &back = _bv.ref_at(_bv.nref - 1);
        if (back.block == r.block && back.offset + back.length == r.offset) {
            // Merge ref
            back.length += r.length;
            _bv.nbytes += r.length;
            if (MOVE) {
                r.block->dec_ref();
            }
            return;
        }

        if (_bv.nref != _bv.capacity()) {
            _bv.ref_at(_bv.nref++) = r;
            _bv.nbytes += r.length;
            if (!MOVE) {
                r.block->inc_ref();
            }
            return;
        }
        // resize, don't modify bv until new_refs is fully assigned
        const uint32_t new_cap = _bv.capacity() * 2;
        BlockRef *new_refs = io_internal::acquire_blockref_array(new_cap);
        for (uint32_t i = 0; i < _bv.nref; ++i) {
            new_refs[i] = _bv.ref_at(i);
        }
        new_refs[_bv.nref++] = r;

        // Change other variables
        _bv.start = 0;
        io_internal::release_blockref_array(_bv.refs, _bv.capacity());
        _bv.refs = new_refs;
        _bv.cap_mask = new_cap - 1;
        _bv.nbytes += r.length;
        if (!MOVE) {
            r.block->inc_ref();
        }
    }

// Explicitly initialize templates.
    template void cord_buf::_push_or_move_back_ref_to_bigview<true>(const BlockRef &);

    template void cord_buf::_push_or_move_back_ref_to_bigview<false>(const BlockRef &);

    template<bool MOVEOUT>
    int cord_buf::_pop_or_moveout_front_ref() {
        if (_small()) {
            if (_sv.refs[0].block != nullptr) {
                if (!MOVEOUT) {
                    _sv.refs[0].block->dec_ref();
                }
                _sv.refs[0] = _sv.refs[1];
                reset_block_ref(_sv.refs[1]);
                return 0;
            }
            return -1;
        } else {
            // _bv.nref must be greater than 2
            const uint32_t start = _bv.start;
            if (!MOVEOUT) {
                _bv.refs[start].block->dec_ref();
            }
            if (--_bv.nref > 2) {
                _bv.start = (start + 1) & _bv.cap_mask;
                _bv.nbytes -= _bv.refs[start].length;
            } else {  // count==2, fall back to SmallView
                BlockRef *const saved_refs = _bv.refs;
                const uint32_t saved_cap_mask = _bv.cap_mask;
                _sv.refs[0] = saved_refs[(start + 1) & saved_cap_mask];
                _sv.refs[1] = saved_refs[(start + 2) & saved_cap_mask];
                io_internal::release_blockref_array(saved_refs, saved_cap_mask + 1);
            }
            return 0;
        }
    }

// Explicitly initialize templates.
    template int cord_buf::_pop_or_moveout_front_ref<true>();

    template int cord_buf::_pop_or_moveout_front_ref<false>();

    int cord_buf::_pop_back_ref() {
        if (_small()) {
            if (_sv.refs[1].block != nullptr) {
                _sv.refs[1].block->dec_ref();
                reset_block_ref(_sv.refs[1]);
                return 0;
            } else if (_sv.refs[0].block != nullptr) {
                _sv.refs[0].block->dec_ref();
                reset_block_ref(_sv.refs[0]);
                return 0;
            }
            return -1;
        } else {
            // _bv.nref must be greater than 2
            const uint32_t start = _bv.start;
            cord_buf::BlockRef &back = _bv.refs[(start + _bv.nref - 1) & _bv.cap_mask];
            back.block->dec_ref();
            if (--_bv.nref > 2) {
                _bv.nbytes -= back.length;
            } else {  // count==2, fall back to SmallView
                BlockRef *const saved_refs = _bv.refs;
                const uint32_t saved_cap_mask = _bv.cap_mask;
                _sv.refs[0] = saved_refs[start];
                _sv.refs[1] = saved_refs[(start + 1) & saved_cap_mask];
                io_internal::release_blockref_array(saved_refs, saved_cap_mask + 1);
            }
            return 0;
        }
    }

    void cord_buf::clear() {
        if (_small()) {
            if (_sv.refs[0].block != nullptr) {
                _sv.refs[0].block->dec_ref();
                reset_block_ref(_sv.refs[0]);

                if (_sv.refs[1].block != nullptr) {
                    _sv.refs[1].block->dec_ref();
                    reset_block_ref(_sv.refs[1]);
                }
            }
        } else {
            for (uint32_t i = 0; i < _bv.nref; ++i) {
                _bv.ref_at(i).block->dec_ref();
            }
            io_internal::release_blockref_array(_bv.refs, _bv.capacity());
            new(this) cord_buf;
        }
    }

    size_t cord_buf::pop_front(size_t n) {
        const size_t len = length();
        if (n >= len) {
            clear();
            return len;
        }
        const size_t saved_n = n;
        while (n) {  // length() == 0 does not enter
            cord_buf::BlockRef &r = _front_ref();
            if (r.length > n) {
                r.offset += n;
                r.length -= n;
                if (!_small()) {
                    _bv.nbytes -= n;
                }
                return saved_n;
            }
            n -= r.length;
            _pop_front_ref();
        }
        return saved_n;
    }

    bool cord_buf::cut1(void *c) {
        if (empty()) {
            return false;
        }
        cord_buf::BlockRef &r = _front_ref();
        *(char *) c = r.block->data[r.offset];
        if (r.length > 1) {
            ++r.offset;
            --r.length;
            if (!_small()) {
                --_bv.nbytes;
            }
        } else {
            _pop_front_ref();
        }
        return true;
    }

    size_t cord_buf::pop_back(size_t n) {
        const size_t len = length();
        if (n >= len) {
            clear();
            return len;
        }
        const size_t saved_n = n;
        while (n) {  // length() == 0 does not enter
            cord_buf::BlockRef &r = _back_ref();
            if (r.length > n) {
                r.length -= n;
                if (!_small()) {
                    _bv.nbytes -= n;
                }
                return saved_n;
            }
            n -= r.length;
            _pop_back_ref();
        }
        return saved_n;
    }

    size_t cord_buf::cutn(cord_buf *out, size_t n) {
        const size_t len = length();
        if (n > len) {
            n = len;
        }
        const size_t saved_n = n;
        while (n) {   // length() == 0 does not enter
            cord_buf::BlockRef &r = _front_ref();
            if (r.length <= n) {
                n -= r.length;
                out->_move_back_ref(r);
                _moveout_front_ref();
            } else {
                const cord_buf::BlockRef cr = {r.offset, (uint32_t) n, r.block};
                out->_push_back_ref(cr);

                r.offset += n;
                r.length -= n;
                if (!_small()) {
                    _bv.nbytes -= n;
                }
                return saved_n;
            }
        }
        return saved_n;
    }

    size_t cord_buf::cutn(void *out, size_t n) {
        const size_t len = length();
        if (n > len) {
            n = len;
        }
        const size_t saved_n = n;
        while (n) {   // length() == 0 does not enter
            cord_buf::BlockRef &r = _front_ref();
            if (r.length <= n) {
                io_internal::cp(out, r.block->data + r.offset, r.length);
                out = (char *) out + r.length;
                n -= r.length;
                _pop_front_ref();
            } else {
                io_internal::cp(out, r.block->data + r.offset, n);
                out = (char *) out + n;
                r.offset += n;
                r.length -= n;
                if (!_small()) {
                    _bv.nbytes -= n;
                }
                return saved_n;
            }
        }
        return saved_n;
    }

    size_t cord_buf::cutn(std::string *out, size_t n) {
        if (n == 0) {
            return 0;
        }
        const size_t len = length();
        if (n > len) {
            n = len;
        }
        const size_t old_size = out->size();
        out->resize(out->size() + n);
        return cutn(&(*out)[old_size], n);
    }

    int cord_buf::_cut_by_char(cord_buf *out, char d) {
        const size_t nref = _ref_num();
        size_t n = 0;

        for (size_t i = 0; i < nref; ++i) {
            cord_buf::BlockRef const &r = _ref_at(i);
            char const *const s = r.block->data + r.offset;
            for (uint32_t j = 0; j < r.length; ++j, ++n) {
                if (s[j] == d) {
                    // There's no way cutn/pop_front fails
                    cutn(out, n);
                    pop_front(1);
                    return 0;
                }
            }
        }

        return -1;
    }

    int cord_buf::_cut_by_delim(cord_buf *out, char const *dbegin, size_t ndelim) {
        typedef unsigned long SigType;
        const size_t NMAX = sizeof(SigType);

        if (ndelim > NMAX || ndelim > length()) {
            return -1;
        }

        SigType dsig = 0;
        for (size_t i = 0; i < ndelim; ++i) {
            dsig = (dsig << CHAR_BIT) | static_cast<SigType>(dbegin[i]);
        }

        const SigType SIGMASK =
                (ndelim == NMAX ? (SigType) -1 : (((SigType) 1 << (ndelim * CHAR_BIT)) - 1));

        const size_t nref = _ref_num();
        SigType sig = 0;
        size_t n = 0;

        for (size_t i = 0; i < nref; ++i) {
            cord_buf::BlockRef const &r = _ref_at(i);
            char const *const s = r.block->data + r.offset;

            for (uint32_t j = 0; j < r.length; ++j, ++n) {
                sig = ((sig << CHAR_BIT) | static_cast<SigType>(s[j])) & SIGMASK;
                if (sig == dsig) {
                    // There's no way cutn/pop_front fails
                    cutn(out, n + 1 - ndelim);
                    pop_front(ndelim);
                    return 0;
                }
            }
        }

        return -1;
    }

// Since cut_into_file_descriptor() allocates iovec on stack, IOV_MAX=1024
// is too large(in the worst case) for fibers with small stacks.
    static const size_t IOBUF_IOV_MAX = 256;

    ssize_t cord_buf::pcut_into_file_descriptor(int fd, off_t offset, size_t size_hint) {
        if (empty()) {
            return 0;
        }

        const size_t nref = std::min(_ref_num(), IOBUF_IOV_MAX);
        struct iovec vec[nref];
        size_t nvec = 0;
        size_t cur_len = 0;

        do {
            cord_buf::BlockRef const &r = _ref_at(nvec);
            vec[nvec].iov_base = r.block->data + r.offset;
            vec[nvec].iov_len = r.length;
            ++nvec;
            cur_len += r.length;
        } while (nvec < nref && cur_len < size_hint);

        ssize_t nw = 0;

        if (offset >= 0) {
            static io_internal::iov_function pwritev_func = io_internal::get_pwritev_func();
            nw = pwritev_func(fd, vec, nvec, offset);
        } else {
            nw = ::writev(fd, vec, nvec);
        }
        if (nw > 0) {
            pop_front(nw);
        }
        return nw;
    }

    ssize_t cord_buf::cut_into_writer(base_writer *writer, size_t size_hint) {
        if (empty()) {
            return 0;
        }
        const size_t nref = std::min(_ref_num(), IOBUF_IOV_MAX);
        struct iovec vec[nref];
        size_t nvec = 0;
        size_t cur_len = 0;

        do {
            cord_buf::BlockRef const &r = _ref_at(nvec);
            vec[nvec].iov_base = r.block->data + r.offset;
            vec[nvec].iov_len = r.length;
            ++nvec;
            cur_len += r.length;
        } while (nvec < nref && cur_len < size_hint);

        const ssize_t nw = writer->writev(vec, nvec);
        if (nw > 0) {
            pop_front(nw);
        }
        return nw;
    }

    ssize_t cord_buf::pcut_multiple_into_file_descriptor(
            int fd, off_t offset, cord_buf *const *pieces, size_t count) {
        if (MELON_UNLIKELY(count == 0)) {
            return 0;
        }
        if (1UL == count) {
            return pieces[0]->pcut_into_file_descriptor(fd, offset);
        }
        struct iovec vec[IOBUF_IOV_MAX];
        size_t nvec = 0;
        for (size_t i = 0; i < count; ++i) {
            const cord_buf *p = pieces[i];
            const size_t nref = p->_ref_num();
            for (size_t j = 0; j < nref && nvec < IOBUF_IOV_MAX; ++j, ++nvec) {
                cord_buf::BlockRef const &r = p->_ref_at(j);
                vec[nvec].iov_base = r.block->data + r.offset;
                vec[nvec].iov_len = r.length;
            }
        }

        ssize_t nw = 0;
        if (offset >= 0) {
            static io_internal::iov_function pwritev_func = io_internal::get_pwritev_func();
            nw = pwritev_func(fd, vec, nvec, offset);
        } else {
            nw = ::writev(fd, vec, nvec);
        }
        if (nw <= 0) {
            return nw;
        }
        size_t npop_all = nw;
        for (size_t i = 0; i < count; ++i) {
            npop_all -= pieces[i]->pop_front(npop_all);
            if (npop_all == 0) {
                break;
            }
        }
        return nw;
    }

    ssize_t cord_buf::cut_multiple_into_writer(
            base_writer *writer, cord_buf *const *pieces, size_t count) {
        if (MELON_UNLIKELY(count == 0)) {
            return 0;
        }
        if (1UL == count) {
            return pieces[0]->cut_into_writer(writer);
        }
        struct iovec vec[IOBUF_IOV_MAX];
        size_t nvec = 0;
        for (size_t i = 0; i < count; ++i) {
            const cord_buf *p = pieces[i];
            const size_t nref = p->_ref_num();
            for (size_t j = 0; j < nref && nvec < IOBUF_IOV_MAX; ++j, ++nvec) {
                cord_buf::BlockRef const &r = p->_ref_at(j);
                vec[nvec].iov_base = r.block->data + r.offset;
                vec[nvec].iov_len = r.length;
            }
        }

        const ssize_t nw = writer->writev(vec, nvec);
        if (nw <= 0) {
            return nw;
        }
        size_t npop_all = nw;
        for (size_t i = 0; i < count; ++i) {
            npop_all -= pieces[i]->pop_front(npop_all);
            if (npop_all == 0) {
                break;
            }
        }
        return nw;
    }


    void cord_buf::append(const cord_buf &other) {
        const size_t nref = other._ref_num();
        for (size_t i = 0; i < nref; ++i) {
            _push_back_ref(other._ref_at(i));
        }
    }

    void cord_buf::append(const Movable &movable_other) {
        if (empty()) {
            swap(movable_other.value());
        } else {
            melon::cord_buf &other = movable_other.value();
            const size_t nref = other._ref_num();
            for (size_t i = 0; i < nref; ++i) {
                _move_back_ref(other._ref_at(i));
            }
            if (!other._small()) {
                io_internal::release_blockref_array(other._bv.refs, other._bv.capacity());
            }
            new(&other) cord_buf;
        }
    }

    int cord_buf::push_back(char c) {
        cord_buf::buf_block *b = io_internal::share_tls_block();
        if (MELON_UNLIKELY(!b)) {
            return -1;
        }
        b->data[b->size] = c;
        const cord_buf::BlockRef r = {b->size, 1, b};
        ++b->size;
        _push_back_ref(r);
        return 0;
    }

    int cord_buf::append(char const *s) {
        if (MELON_LIKELY(s != nullptr)) {
            return append(s, strlen(s));
        }
        return -1;
    }

    int cord_buf::append(void const *data, size_t count) {
        if (MELON_UNLIKELY(!data)) {
            return -1;
        }
        if (count == 1) {
            return push_back(*((char const *) data));
        }
        size_t total_nc = 0;
        while (total_nc < count) {  // excluded count == 0
            cord_buf::buf_block *b = io_internal::share_tls_block();
            if (MELON_UNLIKELY(!b)) {
                return -1;
            }
            const size_t nc = std::min(count - total_nc, b->left_space());
            io_internal::cp(b->data + b->size, (char *) data + total_nc, nc);

            const cord_buf::BlockRef r = {(uint32_t) b->size, (uint32_t) nc, b};
            _push_back_ref(r);
            b->size += nc;
            total_nc += nc;
        }
        return 0;
    }

    int cord_buf::appendv(const const_iovec *vec, size_t n) {
        size_t offset = 0;
        for (size_t i = 0; i < n;) {
            cord_buf::buf_block *b = io_internal::share_tls_block();
            if (MELON_UNLIKELY(!b)) {
                return -1;
            }
            uint32_t total_cp = 0;
            for (; i < n; ++i, offset = 0) {
                const const_iovec &vec_i = vec[i];
                const size_t nc = std::min(vec_i.iov_len - offset, b->left_space() - total_cp);
                io_internal::cp(b->data + b->size + total_cp, (char *) vec_i.iov_base + offset, nc);
                total_cp += nc;
                offset += nc;
                if (offset != vec_i.iov_len) {
                    break;
                }
            }

            const cord_buf::BlockRef r = {(uint32_t) b->size, total_cp, b};
            b->size += total_cp;
            _push_back_ref(r);
        }
        return 0;
    }

    int cord_buf::append_user_data(void *data, size_t size, void (*deleter)(void *)) {
        if (size > 0xFFFFFFFFULL - 100) {
            MELON_LOG(FATAL) << "data_size=" << size << " is too large";
            return -1;
        }
        char *mem = (char *) malloc(sizeof(cord_buf::buf_block) + sizeof(UserDataExtension));
        if (mem == nullptr) {
            return -1;
        }
        if (deleter == nullptr) {
            deleter = ::free;
        }
        cord_buf::buf_block *b = new(mem) cord_buf::buf_block((char *) data, size, deleter);
        const cord_buf::BlockRef r = {0, b->cap, b};
        _move_back_ref(r);
        return 0;
    }

    int cord_buf::resize(size_t n, char c) {
        const size_t saved_len = length();
        if (n < saved_len) {
            pop_back(saved_len - n);
            return 0;
        }
        const size_t count = n - saved_len;
        size_t total_nc = 0;
        while (total_nc < count) {  // excluded count == 0
            cord_buf::buf_block *b = io_internal::share_tls_block();
            if (MELON_UNLIKELY(!b)) {
                return -1;
            }
            const size_t nc = std::min(count - total_nc, b->left_space());
            memset(b->data + b->size, c, nc);

            const cord_buf::BlockRef r = {(uint32_t) b->size, (uint32_t) nc, b};
            _push_back_ref(r);
            b->size += nc;
            total_nc += nc;
        }
        return 0;
    }

    // NOTE: We don't use C++ bitwise fields which make copying slower.
    static const int REF_INDEX_BITS = 19;
    static const int REF_OFFSET_BITS = 15;
    static const int AREA_SIZE_BITS = 30;
    static const uint32_t MAX_REF_INDEX = (((uint32_t) 1) << REF_INDEX_BITS) - 1;
    static const uint32_t MAX_REF_OFFSET = (((uint32_t) 1) << REF_OFFSET_BITS) - 1;
    static const uint32_t MAX_AREA_SIZE = (((uint32_t) 1) << AREA_SIZE_BITS) - 1;

    inline cord_buf::Area make_area(uint32_t ref_index, uint32_t ref_offset,
                                    uint32_t size) {
        if (ref_index > MAX_REF_INDEX ||
            ref_offset > MAX_REF_OFFSET ||
            size > MAX_AREA_SIZE) {
            MELON_LOG(ERROR) << "Too big parameters!";
            return cord_buf::INVALID_AREA;
        }
        return (((uint64_t) ref_index) << (REF_OFFSET_BITS + AREA_SIZE_BITS))
               | (((uint64_t) ref_offset) << AREA_SIZE_BITS)
               | size;
    }

    inline uint32_t get_area_ref_index(cord_buf::Area c) {
        return (c >> (REF_OFFSET_BITS + AREA_SIZE_BITS)) & MAX_REF_INDEX;
    }

    inline uint32_t get_area_ref_offset(cord_buf::Area c) {
        return (c >> AREA_SIZE_BITS) & MAX_REF_OFFSET;
    }

    inline uint32_t get_area_size(cord_buf::Area c) {
        return (c & MAX_AREA_SIZE);
    }

    cord_buf::Area cord_buf::reserve(size_t count) {
        cord_buf::Area result = INVALID_AREA;
        size_t total_nc = 0;
        while (total_nc < count) {  // excluded count == 0
            cord_buf::buf_block *b = io_internal::share_tls_block();
            if (MELON_UNLIKELY(!b)) {
                return INVALID_AREA;
            }
            const size_t nc = std::min(count - total_nc, b->left_space());
            const cord_buf::BlockRef r = {(uint32_t) b->size, (uint32_t) nc, b};
            _push_back_ref(r);
            if (total_nc == 0) {
                // Encode the area at first time. Notice that the pushed ref may
                // be merged with existing ones.
                result = make_area(_ref_num() - 1, _back_ref().length - nc, count);
            }
            total_nc += nc;
            b->size += nc;
        }
        return result;
    }

    int cord_buf::unsafe_assign(Area area, const void *data) {
        if (area == INVALID_AREA || data == nullptr) {
            MELON_LOG(ERROR) << "Invalid parameters";
            return -1;
        }
        const uint32_t ref_index = get_area_ref_index(area);
        uint32_t ref_offset = get_area_ref_offset(area);
        uint32_t length = get_area_size(area);
        const size_t nref = _ref_num();
        for (size_t i = ref_index; i < nref; ++i) {
            cord_buf::BlockRef &r = _ref_at(i);
            // NOTE: we can't check if the block is shared with another cord_buf or
            // not since even a single cord_buf may reference a block multiple times
            // (by different BlockRef-s)

            const size_t nc = std::min(length, r.length - ref_offset);
            io_internal::cp(r.block->data + r.offset + ref_offset, data, nc);
            if (length == nc) {
                return 0;
            }
            ref_offset = 0;
            length -= nc;
            data = (char *) data + nc;
        }

        // Use check because we need to see the stack here.
        MELON_CHECK(false) << "cord_buf(" << size() << ", nref=" << _ref_num()
                           << ") is shorter than what we reserved("
                           << "ref=" << get_area_ref_index(area)
                           << " off=" << get_area_ref_offset(area)
                           << " size=" << get_area_size(area)
                           << "), this assignment probably corrupted something...";
        return -1;
    }

    size_t cord_buf::append_to(cord_buf *buf, size_t n, size_t pos) const {
        const size_t nref = _ref_num();
        // Skip `pos' bytes. `offset' is the starting position in starting BlockRef.
        size_t offset = pos;
        size_t i = 0;
        for (; offset != 0 && i < nref; ++i) {
            cord_buf::BlockRef const &r = _ref_at(i);
            if (offset < (size_t) r.length) {
                break;
            }
            offset -= r.length;
        }
        size_t m = n;
        for (; m != 0 && i < nref; ++i) {
            cord_buf::BlockRef const &r = _ref_at(i);
            const size_t nc = std::min(m, (size_t) r.length - offset);
            const cord_buf::BlockRef r2 = {(uint32_t) (r.offset + offset),
                                           (uint32_t) nc, r.block};
            buf->_push_back_ref(r2);
            offset = 0;
            m -= nc;
        }
        // If nref == 0, here returns 0 correctly
        return n - m;
    }

    size_t cord_buf::copy_to(void *d, size_t n, size_t pos) const {
        const size_t nref = _ref_num();
        // Skip `pos' bytes. `offset' is the starting position in starting BlockRef.
        size_t offset = pos;
        size_t i = 0;
        for (; offset != 0 && i < nref; ++i) {
            cord_buf::BlockRef const &r = _ref_at(i);
            if (offset < (size_t) r.length) {
                break;
            }
            offset -= r.length;
        }
        size_t m = n;
        for (; m != 0 && i < nref; ++i) {
            cord_buf::BlockRef const &r = _ref_at(i);
            const size_t nc = std::min(m, (size_t) r.length - offset);
            io_internal::cp(d, r.block->data + r.offset + offset, nc);
            offset = 0;
            d = (char *) d + nc;
            m -= nc;
        }
        // If nref == 0, here returns 0 correctly
        return n - m;
    }

    size_t cord_buf::copy_to(std::string *s, size_t n, size_t pos) const {
        const size_t len = length();
        if (len <= pos) {
            return 0;
        }
        if (n > len - pos) {  // note: n + pos may overflow
            n = len - pos;
        }
        s->resize(n);
        return copy_to(&(*s)[0], n, pos);
    }

    size_t cord_buf::append_to(std::string *s, size_t n, size_t pos) const {
        const size_t len = length();
        if (len <= pos) {
            return 0;
        }
        if (n > len - pos) {  // note: n + pos may overflow
            n = len - pos;
        }
        const size_t old_size = s->size();
        s->resize(old_size + n);
        return copy_to(&(*s)[old_size], n, pos);
    }


    size_t cord_buf::copy_to_cstr(char *s, size_t n, size_t pos) const {
        const size_t nc = copy_to(s, n, pos);
        s[nc] = '\0';
        return nc;
    }

    void const *cord_buf::fetch(void *d, size_t n) const {
        if (n <= length()) {
            cord_buf::BlockRef const &r0 = _ref_at(0);
            if (n <= r0.length) {
                return r0.block->data + r0.offset;
            }

            io_internal::cp(d, r0.block->data + r0.offset, r0.length);
            size_t total_nc = r0.length;
            const size_t nref = _ref_num();
            for (size_t i = 1; i < nref; ++i) {
                cord_buf::BlockRef const &r = _ref_at(i);
                if (n <= r.length + total_nc) {
                    io_internal::cp((char *) d + total_nc,
                              r.block->data + r.offset, n - total_nc);
                    return d;
                }
                io_internal::cp((char *) d + total_nc, r.block->data + r.offset, r.length);
                total_nc += r.length;
            }
        }
        return nullptr;
    }

    const void *cord_buf::fetch1() const {
        if (!empty()) {
            const cord_buf::BlockRef &r0 = _front_ref();
            return r0.block->data + r0.offset;
        }
        return nullptr;
    }

    std::ostream &operator<<(std::ostream &os, const cord_buf &buf) {
        const size_t n = buf.backing_block_num();
        for (size_t i = 0; i < n; ++i) {
            std::string_view blk = buf.backing_block(i);
            os.write(blk.data(), blk.size());
        }
        return os;
    }

    bool cord_buf::equals(const std::string_view &s) const {
        if (size() != s.size()) {
            return false;
        }
        const size_t nref = _ref_num();
        size_t soff = 0;
        for (size_t i = 0; i < nref; ++i) {
            const BlockRef &r = _ref_at(i);
            if (memcmp(r.block->data + r.offset, s.data() + soff, r.length) != 0) {
                return false;
            }
            soff += r.length;
        }
        return true;
    }

    std::string_view cord_buf::backing_block(size_t i) const {
        if (i < _ref_num()) {
            const BlockRef &r = _ref_at(i);
            return std::string_view(r.block->data + r.offset, r.length);
        }
        return std::string_view();
    }

    bool cord_buf::equals(const melon::cord_buf &other) const {
        const size_t sz1 = size();
        if (sz1 != other.size()) {
            return false;
        }
        if (!sz1) {
            return true;
        }
        const BlockRef &r1 = _ref_at(0);
        const char *d1 = r1.block->data + r1.offset;
        size_t len1 = r1.length;
        const BlockRef &r2 = other._ref_at(0);
        const char *d2 = r2.block->data + r2.offset;
        size_t len2 = r2.length;
        const size_t nref1 = _ref_num();
        const size_t nref2 = other._ref_num();
        size_t i = 1;
        size_t j = 1;
        do {
            const size_t cmplen = std::min(len1, len2);
            if (memcmp(d1, d2, cmplen) != 0) {
                return false;
            }
            len1 -= cmplen;
            if (!len1) {
                if (i >= nref1) {
                    return true;
                }
                const BlockRef &r = _ref_at(i++);
                d1 = r.block->data + r.offset;
                len1 = r.length;
            } else {
                d1 += cmplen;
            }
            len2 -= cmplen;
            if (!len2) {
                if (j >= nref2) {
                    return true;
                }
                const BlockRef &r = other._ref_at(j++);
                d2 = r.block->data + r.offset;
                len2 = r.length;
            } else {
                d2 += cmplen;
            }
        } while (true);
        return true;
    }

////////////////////////////// IOPortal //////////////////
    IOPortal::~IOPortal() { return_cached_blocks(); }

    IOPortal &IOPortal::operator=(const IOPortal &rhs) {
        cord_buf::operator=(rhs);
        return *this;
    }

    void IOPortal::clear() {
        cord_buf::clear();
        return_cached_blocks();
    }

    const int MAX_APPEND_IOVEC = 64;

    ssize_t IOPortal::pappend_from_file_descriptor(
            int fd, off_t offset, size_t max_count) {
        iovec vec[MAX_APPEND_IOVEC];
        int nvec = 0;
        size_t space = 0;
        buf_block *prev_p = nullptr;
        buf_block *p = _block;
        // Prepare at most MAX_APPEND_IOVEC blocks or space of blocks >= max_count
        do {
            if (p == nullptr) {
                p = io_internal::acquire_tls_block();
                if (MELON_UNLIKELY(!p)) {
                    errno = ENOMEM;
                    return -1;
                }
                if (prev_p != nullptr) {
                    prev_p->portal_next = p;
                } else {
                    _block = p;
                }
            }
            vec[nvec].iov_base = p->data + p->size;
            vec[nvec].iov_len = std::min(p->left_space(), max_count - space);
            space += vec[nvec].iov_len;
            ++nvec;
            if (space >= max_count || nvec >= MAX_APPEND_IOVEC) {
                break;
            }
            prev_p = p;
            p = p->portal_next;
        } while (1);

        ssize_t nr = 0;
        if (offset < 0) {
            nr = readv(fd, vec, nvec);
        } else {
            static io_internal::iov_function preadv_func = io_internal::get_preadv_func();
            nr = preadv_func(fd, vec, nvec, offset);
        }
        if (nr <= 0) {  // -1 or 0
            if (empty()) {
                return_cached_blocks();
            }
            return nr;
        }

        size_t total_len = nr;
        do {
            const size_t len = std::min(total_len, _block->left_space());
            total_len -= len;
            const cord_buf::BlockRef r = {_block->size, (uint32_t) len, _block};
            _push_back_ref(r);
            _block->size += len;
            if (_block->full()) {
                buf_block *const saved_next = _block->portal_next;
                _block->dec_ref();  // _block may be deleted
                _block = saved_next;
            }
        } while (total_len);
        return nr;
    }

    ssize_t IOPortal::append_from_reader(base_reader *reader, size_t max_count) {
        iovec vec[MAX_APPEND_IOVEC];
        int nvec = 0;
        size_t space = 0;
        buf_block *prev_p = nullptr;
        buf_block *p = _block;
        // Prepare at most MAX_APPEND_IOVEC blocks or space of blocks >= max_count
        do {
            if (p == nullptr) {
                p = io_internal::acquire_tls_block();
                if (MELON_UNLIKELY(!p)) {
                    errno = ENOMEM;
                    return -1;
                }
                if (prev_p != nullptr) {
                    prev_p->portal_next = p;
                } else {
                    _block = p;
                }
            }
            vec[nvec].iov_base = p->data + p->size;
            vec[nvec].iov_len = std::min(p->left_space(), max_count - space);
            space += vec[nvec].iov_len;
            ++nvec;
            if (space >= max_count || nvec >= MAX_APPEND_IOVEC) {
                break;
            }
            prev_p = p;
            p = p->portal_next;
        } while (1);

        const ssize_t nr = reader->readv(vec, nvec);
        if (nr <= 0) {  // -1 or 0
            if (empty()) {
                return_cached_blocks();
            }
            return nr;
        }

        size_t total_len = nr;
        do {
            const size_t len = std::min(total_len, _block->left_space());
            total_len -= len;
            const cord_buf::BlockRef r = {_block->size, (uint32_t) len, _block};
            _push_back_ref(r);
            _block->size += len;
            if (_block->full()) {
                buf_block *const saved_next = _block->portal_next;
                _block->dec_ref();  // _block may be deleted
                _block = saved_next;
            }
        } while (total_len);
        return nr;
    }
    
    void IOPortal::return_cached_blocks_impl(buf_block *b) {
        io_internal::release_tls_block_chain(b);
    }

//////////////// cord_buf_cutter ////////////////

    cord_buf_cutter::cord_buf_cutter(melon::cord_buf *buf)
            : _data(nullptr), _data_end(nullptr), _block(nullptr), _buf(buf) {
    }

    cord_buf_cutter::~cord_buf_cutter() {
        if (_block) {
            if (_data != _data_end) {
                cord_buf::BlockRef &fr = _buf->_front_ref();
                MELON_CHECK_EQ(fr.block, _block);
                fr.offset = (uint32_t) ((char *) _data - _block->data);
                fr.length = (uint32_t) ((char *) _data_end - (char *) _data);
            } else {
                _buf->_pop_front_ref();
            }
        }
    }

    bool cord_buf_cutter::load_next_ref() {
        if (_block) {
            _buf->_pop_front_ref();
        }
        if (!_buf->_ref_num()) {
            _data = nullptr;
            _data_end = nullptr;
            _block = nullptr;
            return false;
        } else {
            const cord_buf::BlockRef &r = _buf->_front_ref();
            _data = r.block->data + r.offset;
            _data_end = (char *) _data + r.length;
            _block = r.block;
            return true;
        }
    }

    size_t cord_buf_cutter::slower_copy_to(void *dst, size_t n) {
        size_t size = (char *) _data_end - (char *) _data;
        if (size == 0) {
            if (!load_next_ref()) {
                return 0;
            }
            size = (char *) _data_end - (char *) _data;
            if (n <= size) {
                memcpy(dst, _data, n);
                return n;
            }
        }
        void *const saved_dst = dst;
        memcpy(dst, _data, size);
        dst = (char *) dst + size;
        n -= size;
        const size_t nref = _buf->_ref_num();
        for (size_t i = 1; i < nref; ++i) {
            const cord_buf::BlockRef &r = _buf->_ref_at(i);
            const size_t nc = std::min(n, (size_t) r.length);
            memcpy(dst, r.block->data + r.offset, nc);
            dst = (char *) dst + nc;
            n -= nc;
            if (n == 0) {
                break;
            }
        }
        return (char *) dst - (char *) saved_dst;
    }

    size_t cord_buf_cutter::cutn(melon::cord_buf *out, size_t n) {
        if (n == 0) {
            return 0;
        }
        const size_t size = (char *) _data_end - (char *) _data;
        if (n <= size) {
            const cord_buf::BlockRef r = {(uint32_t) ((char *) _data - _block->data),
                                          (uint32_t) n,
                                          _block};
            out->_push_back_ref(r);
            _data = (char *) _data + n;
            return n;
        } else if (size != 0) {
            const cord_buf::BlockRef r = {(uint32_t) ((char *) _data - _block->data),
                                          (uint32_t) size,
                                          _block};
            out->_push_back_ref(r);
            _buf->_pop_front_ref();
            _data = nullptr;
            _data_end = nullptr;
            _block = nullptr;
            return _buf->cutn(out, n - size) + size;
        } else {
            if (_block) {
                _data = nullptr;
                _data_end = nullptr;
                _block = nullptr;
                _buf->_pop_front_ref();
            }
            return _buf->cutn(out, n);
        }
    }

    size_t cord_buf_cutter::cutn(void *out, size_t n) {
        if (n == 0) {
            return 0;
        }
        const size_t size = (char *) _data_end - (char *) _data;
        if (n <= size) {
            memcpy(out, _data, n);
            _data = (char *) _data + n;
            return n;
        } else if (size != 0) {
            memcpy(out, _data, size);
            _buf->_pop_front_ref();
            _data = nullptr;
            _data_end = nullptr;
            _block = nullptr;
            return _buf->cutn((char *) out + size, n - size) + size;
        } else {
            if (_block) {
                _data = nullptr;
                _data_end = nullptr;
                _block = nullptr;
                _buf->_pop_front_ref();
            }
            return _buf->cutn(out, n);
        }
    }


    size_t cord_buf_bytes_iterator::append_and_forward(melon::cord_buf *buf, size_t n) {
        size_t nc = 0;
        while (nc < n && _bytes_left != 0) {
            const cord_buf::BlockRef &r = _buf->_ref_at(_block_count - 1);
            const size_t block_size = _block_end - _block_begin;
            const size_t to_copy = std::min(block_size, n - nc);
            cord_buf::BlockRef r2 = {(uint32_t) (_block_begin - r.block->data),
                                     (uint32_t) to_copy, r.block};
            buf->_push_back_ref(r2);
            _block_begin += to_copy;
            _bytes_left -= to_copy;
            nc += to_copy;
            if (_block_begin == _block_end) {
                try_next_block();
            }
        }
        return nc;
    }

    bool cord_buf_bytes_iterator::forward_one_block(const void **data, size_t *size) {
        if (_bytes_left == 0) {
            return false;
        }
        const size_t block_size = _block_end - _block_begin;
        *data = _block_begin;
        *size = block_size;
        _bytes_left -= block_size;
        try_next_block();
        return true;
    }

}  // namespace melon

void *fast_memcpy(void *__restrict dest, const void *__restrict src, size_t n) {
    return melon::io_internal::cp(dest, src, n);
}
