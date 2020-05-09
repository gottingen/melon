
#ifndef ABEL_MEMORY_TLS_SLAB_INL_H_
#define ABEL_MEMORY_TLS_SLAB_INL_H_

#include <iostream>                      // std::ostream
#include <algorithm>                     // std::max, std::min
#include <atomic>
#include <abel/base/profile.h>
#include <abel/thread/mutex.h>
#include <abel/thread/this_thread.h>
#include <vector>

ABEL_DISABLE_CLANG_WARNING(-Wzero-length-array)
namespace abel {

    template<typename T>
    struct item_id {
        uint64_t value;

        operator uint64_t() const {
            return value;
        }

        template<typename U>
        item_id<U> cast() const {
            item_id<U> id = {value};
            return id;
        }
    };

    template<typename T, size_t NITEM>
    struct tls_slab_free_chunk {
        size_t nfree;
        item_id<T> ids[NITEM];
    };

    template<typename T>
    struct tls_slab_free_chunk<T, 0> {
        size_t nfree;
        item_id<T> ids[0];
    };

    struct tls_slab_info {
        size_t local_pool_num;
        size_t block_group_num;
        size_t block_num;
        size_t item_num;
        size_t block_item_num;
        size_t free_chunk_item_num;
        size_t total_size;
        size_t free_item_num;
    };

    static const size_t RP_MAX_BLOCK_NGROUP = 65536;
    static const size_t RP_GROUP_NBLOCK_NBIT = 16;
    static const size_t RP_GROUP_NBLOCK = (1UL << RP_GROUP_NBLOCK_NBIT);
    static const size_t RP_INITIAL_FREE_LIST_SIZE = 1024;

    template<typename T>
    class tls_slab_block_item_num {
        static const size_t N1 = tls_slab_block_max_size<T>::value / sizeof(T);
        static const size_t N2 = (N1 < 1 ? 1 : N1);
    public:
        static const size_t value = (N2 > tls_slab_block_max_item<T>::value ?
                                     tls_slab_block_max_item<T>::value : N2);
    };


    template<typename T>
    class ABEL_CACHE_LINE_ALIGNED tls_slab{
            public:
            static const size_t BLOCK_NITEM = tls_slab_block_item_num<T>::value;
            static const size_t FREE_CHUNK_NITEM = BLOCK_NITEM;

            // Free identifiers are batched in a free_chunk before they're added to
            // global list(_free_chunks).
            typedef tls_slab_free_chunk<T, FREE_CHUNK_NITEM>      free_chunk;
            typedef tls_slab_free_chunk<T, 0> dynamic_free_chunk;

            // When a thread needs memory, it allocates a Block. To improve locality,
            // items in the Block are only used by the thread.
            // To support cache-aligned objects, align Block.items by cacheline.
            struct ABEL_CACHE_LINE_ALIGNED Block {
                char items[sizeof(T) * BLOCK_NITEM];
                size_t nitem;

                Block() : nitem(0)
                {}
            };

            // A Resource addresses at most RP_MAX_BLOCK_NGROUP BlockGroups,
            // each block_group addresses at most RP_GROUP_NBLOCK blocks. So a
            // resource addresses at most RP_MAX_BLOCK_NGROUP * RP_GROUP_NBLOCK Blocks.
            struct block_group {
                std::atomic<size_t> nblock;
                std::atomic<Block *> blocks[RP_GROUP_NBLOCK];

                block_group() : nblock(0)
                {
                    // We fetch_add nblock in add_block() before setting the entry,
                    // thus address_resource() may sees the unset entry. Initialize
                    // all entries to NULL makes such address_resource() return NULL.
                    memset(blocks, 0, sizeof(std::atomic<Block *>) * RP_GROUP_NBLOCK);
                }
            };


            // Each thread has an instance of this class.
            class ABEL_CACHE_LINE_ALIGNED local_slab {
                public:
                explicit local_slab(tls_slab
                *pool)
                : _pool(pool)
                        , _cur_block(NULL)
                        , _cur_block_index(0)
                {
                    _cur_free.nfree = 0;
                }

                ~local_slab()
                {
                    // Add to global _free_chunks if there're some free resources
                    if (_cur_free.nfree) {
                        _pool->push_free_chunk(_cur_free);
                    }

                    _pool->clear_from_destructor_of_local_pool();
                }

                static void delete_local_pool(void *arg) {
                    delete (local_slab *) arg;
                }

                // We need following macro to construct T with different CTOR_ARGS
                // which may include parenthesis because when T is POD, "new T()"
                // and "new T" are different: former one sets all fields to 0 which
                // we don't want.
#define ABEL_TLS_SLAB_GET(CTOR_ARGS)                              \
        /* Fetch local free id */                                       \
        if (_cur_free.nfree) {                                          \
            const item_id<T> free_id = _cur_free.ids[--_cur_free.nfree]; \
            *id = free_id;                                              \
            _global_nfree.fetch_sub(1, std::memory_order_relaxed);                   \
            return unsafe_address_resource(free_id);                    \
        }                                                               \
        /* Fetch a free_chunk from global.                               \
           TODO: Popping from _free needs to copy a free_chunk which is  \
           costly, but hardly impacts amortized performance. */         \
        if (_pool->pop_free_chunk(_cur_free)) {                         \
            --_cur_free.nfree;                                          \
            const item_id<T> free_id =  _cur_free.ids[_cur_free.nfree]; \
            *id = free_id;                                              \
            _global_nfree.fetch_sub(1, std::memory_order_relaxed);                   \
            return unsafe_address_resource(free_id);                    \
        }                                                               \
        /* Fetch memory from local block */                             \
        if (_cur_block && _cur_block->nitem < BLOCK_NITEM) {            \
            id->value = _cur_block_index * BLOCK_NITEM + _cur_block->nitem; \
            T* p = new ((T*)_cur_block->items + _cur_block->nitem) T CTOR_ARGS; \
            if (!tls_slab_validator<T>::validate(p)) {               \
                p->~T();                                                \
                return NULL;                                            \
            }                                                           \
            ++_cur_block->nitem;                                        \
            return p;                                                   \
        }                                                               \
        /* Fetch a Block from global */                                 \
        _cur_block = add_block(&_cur_block_index);                      \
        if (_cur_block != NULL) {                                       \
            id->value = _cur_block_index * BLOCK_NITEM + _cur_block->nitem; \
            T* p = new ((T*)_cur_block->items + _cur_block->nitem) T CTOR_ARGS; \
            if (!tls_slab_validator<T>::validate(p)) {               \
                p->~T();                                                \
                return NULL;                                            \
            }                                                           \
            ++_cur_block->nitem;                                        \
            return p;                                                   \
        }                                                               \
        return NULL;                                                    \


                inline T *get(item_id<T> *id) {
                    ABEL_TLS_SLAB_GET();
                }

                template<typename A1>
                inline T *get(item_id<T> *id, const A1 &a1) {
                    ABEL_TLS_SLAB_GET((a1));
                }

                template<typename A1, typename A2>
                inline T *get(item_id<T> *id, const A1 &a1, const A2 &a2) {
                    ABEL_TLS_SLAB_GET((a1, a2));
                }

#undef ABEL_TLS_SLAB_GET

                inline int return_resource(item_id<T> id) {
                    // Return to local free list
                    if (_cur_free.nfree < tls_slab::free_chunk_nitem()) {
                        _cur_free.ids[_cur_free.nfree++] = id;
                        _global_nfree.fetch_add(1, std::memory_order_relaxed);
                        return 0;
                    }
                    // Local free list is full, return it to global.
                    // For copying issue, check comment in upper get()
                    if (_pool->push_free_chunk(_cur_free)) {
                        _cur_free.nfree = 1;
                        _cur_free.ids[0] = id;
                        _global_nfree.fetch_add(1, std::memory_order_relaxed);
                        return 0;
                    }
                    return -1;
                }

                private:
                tls_slab * _pool;
                Block *_cur_block;
                size_t _cur_block_index;
                free_chunk _cur_free;
            };

            static inline T* unsafe_address_resource(item_id<T> id) {
                const size_t block_index = id.value / BLOCK_NITEM;
                return (T *) (_block_groups[(block_index >> RP_GROUP_NBLOCK_NBIT)]
                        .load(std::memory_order_consume)
                        ->blocks[(block_index & (RP_GROUP_NBLOCK - 1))]
                        .load(std::memory_order_consume)->items) +
                       id.value - block_index * BLOCK_NITEM;
            }

            static inline T* address_resource(item_id<T> id) {
                const size_t block_index = id.value / BLOCK_NITEM;
                const size_t group_index = (block_index >> RP_GROUP_NBLOCK_NBIT);
                if (__builtin_expect(group_index < RP_MAX_BLOCK_NGROUP, 1)) {
                    block_group *bg =
                            _block_groups[group_index].load(std::memory_order_consume);
                    if (__builtin_expect(bg != NULL, 1)) {
                        Block *b = bg->blocks[block_index & (RP_GROUP_NBLOCK - 1)]
                                .load(std::memory_order_consume);
                        if (__builtin_expect(b != NULL, 1)) {
                            const size_t offset = id.value - block_index * BLOCK_NITEM;
                            if (__builtin_expect(offset < b->nitem, 1)) {
                                return (T *) b->items + offset;
                            }
                        }
                    }
                }

                return NULL;
            }

            inline T* get_resource(item_id<T>* id) {
                local_slab *lp = get_or_new_local_pool();
                if (__builtin_expect(lp != NULL, 1)) {
                    return lp->get(id);
                }
                return NULL;
            }

            template <typename A1>
            inline T* get_resource(item_id<T>* id, const A1& arg1) {
                local_slab *lp = get_or_new_local_pool();
                if (__builtin_expect(lp != NULL, 1)) {
                    return lp->get(id, arg1);
                }
                return NULL;
            }

            template <typename A1, typename A2>
            inline T* get_resource(item_id<T>* id, const A1& arg1, const A2& arg2) {
                local_slab *lp = get_or_new_local_pool();
                if (__builtin_expect(lp != NULL, 1)) {
                    return lp->get(id, arg1, arg2);
                }
                return NULL;
            }

            inline int return_resource(item_id<T> id) {
                local_slab *lp = get_or_new_local_pool();
                if (__builtin_expect(lp != NULL, 1)) {
                    return lp->return_resource(id);
                }
                return -1;
            }

            void clear_resources() {
                local_slab *lp = _local_pool;
                if (lp) {
                    _local_pool = NULL;
                    abel::thread_atexit_cancel(local_slab::delete_local_pool, lp);
                    delete lp;
                }
            }

            static inline size_t free_chunk_nitem() {
                const size_t n = tls_slab_block_max_free_chunk<T>::value();
                return n < FREE_CHUNK_NITEM ? n : FREE_CHUNK_NITEM;
            }

            // Number of all allocated objects, including being used and free.
            tls_slab_info describe_resources() const {
                tls_slab_info info;
                info.local_pool_num = _nlocal.load(std::memory_order_relaxed);
                info.block_group_num = _ngroup.load(std::memory_order_acquire);
                info.block_num = 0;
                info.item_num = 0;
                info.free_chunk_item_num = free_chunk_nitem();
                info.block_item_num = BLOCK_NITEM;
                info.free_item_num = _global_nfree.load(std::memory_order_relaxed);

                for (size_t i = 0; i < info.block_group_num; ++i) {
                    block_group *bg = _block_groups[i].load(std::memory_order_consume);
                    if (NULL == bg) {
                        break;
                    }
                    size_t nblock = std::min(bg->nblock.load(std::memory_order_relaxed),
                                             RP_GROUP_NBLOCK);
                    info.block_num += nblock;
                    for (size_t j = 0; j < nblock; ++j) {
                        Block *b = bg->blocks[j].load(std::memory_order_consume);
                        if (NULL != b) {
                            info.item_num += b->nitem;
                        }
                    }
                }
                info.total_size = info.block_num * info.block_item_num * sizeof(T);
                return info;
            }

            static inline tls_slab* singleton() {
                tls_slab *p = _singleton.load(std::memory_order_consume);
                if (p) {
                    return p;
                }

                _singleton_mutex.lock();
                p = _singleton.load(std::memory_order_consume);
                if (!p) {
                    p = new tls_slab();
                    _singleton.store(p, std::memory_order_release);
                }
                _singleton_mutex.unlock();
                return p;
            }

            private:
            tls_slab() {
                _free_chunks.reserve(RP_INITIAL_FREE_LIST_SIZE);
            }

            ~tls_slab() {
            }

            // Create a Block and append it to right-most block_group.
            static Block* add_block(size_t* index) {
                Block *const new_block = new(std::nothrow) Block;
                if (NULL == new_block) {
                    return NULL;
                }

                size_t ngroup;
                do {
                    ngroup = _ngroup.load(std::memory_order_acquire);
                    if (ngroup >= 1) {
                        block_group *const g =
                                _block_groups[ngroup - 1].load(std::memory_order_consume);
                        const size_t block_index =
                                g->nblock.fetch_add(1, std::memory_order_relaxed);
                        if (block_index < RP_GROUP_NBLOCK) {
                            g->blocks[block_index].store(
                                    new_block, std::memory_order_release);
                            *index = (ngroup - 1) * RP_GROUP_NBLOCK + block_index;
                            return new_block;
                        }
                        g->nblock.fetch_sub(1, std::memory_order_relaxed);
                    }
                } while (add_block_group(ngroup));

                // Fail to add_block_group.
                delete new_block;
                return NULL;
            }

            // Create a block_group and append it to _block_groups.
            // Shall be called infrequently because a block_group is pretty big.
            static bool add_block_group(size_t old_ngroup) {
                block_group *bg = NULL;
                abel::mutex_lock mu(&_block_group_mutex);
                const size_t ngroup = _ngroup.load(std::memory_order_acquire);
                if (ngroup != old_ngroup) {
                    // Other thread got lock and added group before this thread.
                    return true;
                }
                if (ngroup < RP_MAX_BLOCK_NGROUP) {
                    bg = new(std::nothrow) block_group;
                    if (NULL != bg) {
                        // Release fence is paired with consume fence in address() and
                        // add_block() to avoid un-constructed bg to be seen by other
                        // threads.
                        _block_groups[ngroup].store(bg, std::memory_order_release);
                        _ngroup.store(ngroup + 1, std::memory_order_release);
                    }
                }
                return bg != NULL;
            }

            inline local_slab* get_or_new_local_pool() {
                local_slab *lp = _local_pool;
                if (lp != NULL) {
                    return lp;
                }
                lp = new(std::nothrow) local_slab(this);
                if (NULL == lp) {
                    return NULL;
                }
                abel::mutex_lock ml(&_change_thread_mutex);
                _local_pool = lp;
                abel::thread_atexit(local_slab::delete_local_pool, lp);
                _nlocal.fetch_add(1, std::memory_order_relaxed);
                return lp;
            }

            void clear_from_destructor_of_local_pool() {
                // Remove tls
                _local_pool = NULL;

                if (_nlocal.fetch_sub(1, std::memory_order_relaxed) != 1) {
                    return;
                }

                // Can't delete global even if all threads(called tls_slab
                // functions) quit because the memory may still be referenced by
                // other threads. But we need to validate that all memory can
                // be deallocated correctly in tests, so wrap the function with
                // a macro which is only defined in unittests.
                abel::mutex_lock mu(&_change_thread_mutex);
                // Do nothing if there're active threads.
                if (_nlocal.load(std::memory_order_relaxed) != 0) {
                    return;
                }
                // All threads exited and we're holding _change_thread_mutex to avoid
                // racing with new threads calling get_resource().

                // Clear global free list.
                free_chunk dummy;
                while (pop_free_chunk(dummy));

                // Delete all memory
                const size_t ngroup = _ngroup.exchange(0, std::memory_order_relaxed);
                for (size_t i = 0; i < ngroup; ++i) {
                    block_group *bg = _block_groups[i].load(std::memory_order_relaxed);
                    if (NULL == bg) {
                        break;
                    }
                    size_t nblock = std::min(bg->nblock.load(std::memory_order_relaxed),
                                             RP_GROUP_NBLOCK);
                    for (size_t j = 0; j < nblock; ++j) {
                        Block *b = bg->blocks[j].load(std::memory_order_relaxed);
                        if (NULL == b) {
                            continue;
                        }
                        for (size_t k = 0; k < b->nitem; ++k) {
                            T *const objs = (T *) b->items;
                            objs[k].~T();
                        }
                        delete b;
                    }
                    delete bg;
                }

                memset(_block_groups, 0, sizeof(block_group * ) * RP_MAX_BLOCK_NGROUP);
            }

            private:
            bool pop_free_chunk(free_chunk& c) {
                // Critical for the case that most return_object are called in
                // different threads of get_object.
                if (_free_chunks.empty()) {
                    return false;
                }
                _free_chunks_mutex.lock();
                if (_free_chunks.empty()) {
                    _free_chunks_mutex.unlock();
                    return false;
                }
                dynamic_free_chunk *p = _free_chunks.back();
                _free_chunks.pop_back();
                _free_chunks_mutex.unlock();
                c.nfree = p->nfree;
                memcpy(c.ids, p->ids, sizeof(*p->ids) * p->nfree);
                free(p);
                return true;
            }

            bool push_free_chunk(const free_chunk& c) {
                dynamic_free_chunk *p = (dynamic_free_chunk *) malloc(
                        offsetof(dynamic_free_chunk, ids) + sizeof(*c.ids) * c.nfree);
                if (!p) {
                    return false;
                }
                p->nfree = c.nfree;
                memcpy(p->ids, c.ids, sizeof(*c.ids) * c.nfree);
                _free_chunks_mutex.lock();
                _free_chunks.push_back(p);
                _free_chunks_mutex.unlock();
                return true;
            }

            static std::atomic<tls_slab*> _singleton;
            static abel::mutex _singleton_mutex;
            static ABEL_THREAD_LOCAL local_slab* _local_pool;
            static std::atomic<long> _nlocal;
            static std::atomic<size_t> _ngroup;
            static abel::mutex _block_group_mutex;
            static abel::mutex _change_thread_mutex;
            static std::atomic<block_group*> _block_groups[RP_MAX_BLOCK_NGROUP];

            std::vector<dynamic_free_chunk*> _free_chunks;
            abel::mutex _free_chunks_mutex;

            static std::atomic<size_t> _global_nfree;
    };

// Declare template static variables:

    template<typename T>
    const size_t tls_slab<T>::FREE_CHUNK_NITEM;

    template<typename T>
    ABEL_THREAD_LOCAL typename tls_slab<T>::local_slab
            *tls_slab<T>::_local_pool = NULL;

    template<typename T>
    std::atomic<tls_slab<T> *> tls_slab<T>::_singleton;

    template<typename T>
    abel::mutex tls_slab<T>::_singleton_mutex;

    template<typename T>
    std::atomic<long> tls_slab<T>::_nlocal;

    template<typename T>
    std::atomic<size_t> tls_slab<T>::_ngroup;

    template<typename T>
    abel::mutex tls_slab<T>::_block_group_mutex;

    template<typename T>
    abel::mutex tls_slab<T>::_change_thread_mutex;

    template<typename T>
    std::atomic<typename tls_slab<T>::block_group *>
            tls_slab<T>::_block_groups[RP_MAX_BLOCK_NGROUP] = {};

    template<typename T>
    std::atomic<size_t> tls_slab<T>::_global_nfree{};

    template<typename T>
    inline bool operator==(item_id<T> id1, item_id<T> id2) {
        return id1.value == id2.value;
    }

    template<typename T>
    inline bool operator!=(item_id<T> id1, item_id<T> id2) {
        return id1.value != id2.value;
    }

// Disable comparisons between different typed item_id
    template<typename T1, typename T2>
    bool operator==(item_id<T1> id1, item_id<T2> id2);

    template<typename T1, typename T2>
    bool operator!=(item_id<T1> id1, item_id<T2> id2);

    inline std::ostream &operator<<(std::ostream &os,
                                    tls_slab_info const &info) {
        return os << "local_pool_num: " << info.local_pool_num
                  << "\nblock_group_num: " << info.block_group_num
                  << "\nblock_num: " << info.block_num
                  << "\nitem_num: " << info.item_num
                  << "\nblock_item_num: " << info.block_item_num
                  << "\nfree_chunk_item_num: " << info.free_chunk_item_num
                  << "\ntotal_size: " << info.total_size
                  << "\nfree_num: " << info.free_item_num;
    }

}  // namespace abel
ABEL_RESTORE_CLANG_WARNING()
#endif  // ABEL_MEMORY_TLS_SLAB_INL_H_s
