// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//

// A low-level allocator that can be used by other low-level
// modules without introducing dependency cycles.
// This allocator is slow and wasteful of memory;
// it should not be used when performance is key.

#include "abel/memory/internal/low_level_alloc.h"
#include <type_traits>
#include "abel/thread/call_once.h"
#include "abel/base/profile.h"
#include "abel/memory/internal/direct_mmap.h"
#include "abel/thread/internal/scheduling_mode.h"
#include "abel/base/profile.h"
#include "abel/thread/thread_annotations.h"

// low_level_alloc requires that the platform support low-level
// allocation of virtual memory. Platforms lacking this cannot use
// low_level_alloc.
#ifndef ABEL_LOW_LEVEL_ALLOC_MISSING

#ifndef _WIN32

#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

#else
#include <windows.h>
#endif

#include <string.h>
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstddef>
#include <new>                   // for placement-new
#include "abel/thread/dynamic_annotations.h"
#include "abel/log/logging.h"
#include "abel/thread/spin_lock.h"

// MAP_ANONYMOUS
#if defined(__APPLE__)
// For mmap, Linux defines both MAP_ANONYMOUS and MAP_ANON and says MAP_ANON is
// deprecated. In Darwin, MAP_ANON is all there is.
#if !defined MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif  // !MAP_ANONYMOUS
#endif  // __APPLE__

namespace abel {

namespace memory_internal {

// A first-fit allocator with amortized logarithmic free() time.

// ---------------------------------------------------------------------------
static const int kMaxLevel = 30;

namespace {
// This struct describes one allocated block, or one free block.
struct alloc_list {
    struct Header {
        // Size of entire region, including this field. Must be
        // first. Valid in both allocated and unallocated blocks.
        uintptr_t size;

        // kMagicAllocated or kMagicUnallocated xor this.
        uintptr_t magic;

        // Pointer to parent arena.
        low_level_alloc::arena *arena;

        // Aligns regions to 0 mod 2*sizeof(void*).
        void *dummy_for_alignment;
    } header;

    // Next two fields: in unallocated blocks: freelist skiplist data
    //                  in allocated blocks: overlaps with client data

    // Levels in skiplist used.
    int levels;

    // Actually has levels elements. The alloc_list node may not have room
    // for all kMaxLevel entries. See max_fit in lla_skip_list_levels().
    alloc_list *next[kMaxLevel];
};
}  // namespace

// ---------------------------------------------------------------------------
// A trivial skiplist implementation.  This is used to keep the freelist
// in address order while taking only logarithmic time per insert and delete.

// An integer approximation of log2(size/base)
// Requires size >= base.
static int IntLog2(size_t size, size_t base) {
    int result = 0;
    for (size_t i = size; i > base; i >>= 1) {  // i == floor(size/2**result)
        result++;
    }
    //    floor(size / 2**result) <= base < floor(size / 2**(result-1))
    // =>     log2(size/(base+1)) <= result < 1+log2(size/base)
    // => result ~= log2(size/base)
    return result;
}

// Return a random integer n:  p(n)=1/(2**n) if 1 <= n; p(n)=0 if n < 1.
static int Random(uint32_t *state) {
    uint32_t r = *state;
    int result = 1;
    while ((((r = r * 1103515245 + 12345) >> 30) & 1) == 0) {
        result++;
    }
    *state = r;
    return result;
}

// Return a number of skiplist levels for a node of size bytes, where
// base is the minimum node size.  Compute level=log2(size / base)+n
// where n is 1 if random is false and otherwise a random number generated with
// the standard distribution for a skiplist:  See Random() above.
// Bigger nodes tend to have more skiplist levels due to the log2(size / base)
// term, so first-fit searches touch fewer nodes.  "level" is clipped so
// level<kMaxLevel and next[level-1] will fit in the node.
// 0 < lla_skip_list_levels(x,y,false) <= lla_skip_list_levels(x,y,true) < kMaxLevel
static int lla_skip_list_levels(size_t size, size_t base, uint32_t *random) {
    // max_fit is the maximum number of levels that will fit in a node for the
    // given size.   We can't return more than max_fit, no matter what the
    // random number generator says.
    size_t max_fit = (size - offsetof(alloc_list, next)) / sizeof(alloc_list *);
    int level = IntLog2(size, base) + (random != nullptr ? Random(random) : 1);
    if (static_cast<size_t>(level) > max_fit)
        level = static_cast<int>(max_fit);
    if (level > kMaxLevel - 1)
        level = kMaxLevel - 1;
    DCHECK_MSG(level >= 1, "block not big enough for even one level");
    return level;
}

// Return "atleast", the first element of alloc_list *head s.t. *atleast >= *e.
// For 0 <= i < head->levels, set prev[i] to "no_greater", where no_greater
// points to the last element at level i in the alloc_list less than *e, or is
// head if no such element exists.
static alloc_list *lla_skip_list_search(alloc_list *head,
                                        alloc_list *e, alloc_list **prev) {
    alloc_list *p = head;
    for (int level = head->levels - 1; level >= 0; level--) {
        for (alloc_list *n; (n = p->next[level]) != nullptr && n < e; p = n) {
        }
        prev[level] = p;
    }
    return (head->levels == 0) ? nullptr : prev[0]->next[0];
}

// Insert element *e into alloc_list *head.  Set prev[] as lla_skip_list_search.
// Requires that e->levels be previously set by the caller (using
// lla_skip_list_levels())
static void lla_skip_list_insert(alloc_list *head, alloc_list *e,
                                 alloc_list **prev) {
    lla_skip_list_search(head, e, prev);
    for (; head->levels < e->levels; head->levels++) {  // extend prev pointers
        prev[head->levels] = head;                        // to all *e's levels
    }
    for (int i = 0; i != e->levels; i++) {  // add element to list
        e->next[i] = prev[i]->next[i];
        prev[i]->next[i] = e;
    }
}

// Remove element *e from alloc_list *head.  Set prev[] as lla_skip_list_search().
// Requires that e->levels be previous set by the caller (using
// lla_skip_list_levels())
static void LLA_SkiplistDelete(alloc_list *head, alloc_list *e,
                               alloc_list **prev) {
    alloc_list *found = lla_skip_list_search(head, e, prev);
    DCHECK_MSG(e == found, "element not in freelist");
    for (int i = 0; i != e->levels && prev[i]->next[i] == e; i++) {
        prev[i]->next[i] = e->next[i];
    }
    while (head->levels > 0 && head->next[head->levels - 1] == nullptr) {
        head->levels--;   // reduce head->levels if level unused
    }
}

// ---------------------------------------------------------------------------
// arena implementation

// Metadata for an low_level_alloc arena instance.
struct low_level_alloc::arena {
    // Constructs an arena with the given low_level_alloc flags.
    explicit arena(uint32_t flags_value);

    abel::spin_lock mu;
    // Head of free list, sorted by address
    alloc_list freelist ABEL_GUARDED_BY(mu);
    // Count of allocated blocks
    int32_t allocation_count ABEL_GUARDED_BY(mu);
    // flags passed to new_arena
    const uint32_t flags;
    // Result of sysconf(_SC_PAGESIZE)
    const size_t pagesize;
    // Lowest power of two >= max(16, sizeof(alloc_list))
    const size_t round_up;
    // Smallest allocation block size
    const size_t min_size;
    // PRNG state
    uint32_t random ABEL_GUARDED_BY(mu);
};

namespace {
using arena_storage = std::aligned_storage<sizeof(low_level_alloc::arena),
        alignof(low_level_alloc::arena)>::type;

// Static storage space for the lazily-constructed, default global arena
// instances.  We require this space because the whole point of low_level_alloc
// is to avoid relying on malloc/new.
arena_storage default_arena_storage;
arena_storage unhooked_arena_storage;
#ifndef ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING
arena_storage unhooked_async_sig_safe_arena_storage;
#endif

// We must use low_level_call_once here to construct the global arenas, rather than
// using function-level statics, to avoid recursively invoking the scheduler.
abel::once_flag create_globals_once;

void create_global_arenas() {
    new(&default_arena_storage)
            low_level_alloc::arena(low_level_alloc::kCallMallocHook);
    new(&unhooked_arena_storage) low_level_alloc::arena(0);
#ifndef ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING
    new(&unhooked_async_sig_safe_arena_storage)
            low_level_alloc::arena(low_level_alloc::kAsyncSignalSafe);
#endif
}

// Returns a global arena that does not call into hooks.  Used by new_arena()
// when kCallMallocHook is not set.
low_level_alloc::arena *UnhookedArena() {
    base_internal::low_level_call_once(&create_globals_once, create_global_arenas);
    return reinterpret_cast<low_level_alloc::arena *>(&unhooked_arena_storage);
}

#ifndef ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING

// Returns a global arena that is async-signal safe.  Used by new_arena() when
// kAsyncSignalSafe is set.
low_level_alloc::arena *unhooked_async_sig_safe_arena() {
    base_internal::low_level_call_once(&create_globals_once, create_global_arenas);
    return reinterpret_cast<low_level_alloc::arena *>(
            &unhooked_async_sig_safe_arena_storage);
}

#endif

}  // namespace

// Returns the default arena, as used by low_level_alloc::Alloc() and friends.
low_level_alloc::arena *low_level_alloc::default_arena() {
    base_internal::low_level_call_once(&create_globals_once, create_global_arenas);
    return reinterpret_cast<low_level_alloc::arena *>(&default_arena_storage);
}

// magic numbers to identify allocated and unallocated blocks
static const uintptr_t kMagicAllocated = 0x4c833e95U;
static const uintptr_t kMagicUnallocated = ~kMagicAllocated;

namespace {
class ABEL_SCOPED_LOCKABLE arena_lock {
  public:
    explicit arena_lock(low_level_alloc::arena *arena)
    ABEL_EXCLUSIVE_LOCK_FUNCTION(arena->mu)
            : arena_(arena) {
#ifndef ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING
        if ((arena->flags & low_level_alloc::kAsyncSignalSafe) != 0) {
            sigset_t all;
            sigfillset(&all);
            mask_valid_ = pthread_sigmask(SIG_BLOCK, &all, &mask_) == 0;
        }
#endif
        arena_->mu.lock();
    }

    ~arena_lock() { DCHECK_MSG(left_, "haven't left arena region"); }

    void Leave() ABEL_UNLOCK_FUNCTION() {
        arena_->mu.unlock();
#ifndef ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING
        if (mask_valid_) {
            const int err = pthread_sigmask(SIG_SETMASK, &mask_, nullptr);
            if (err != 0) {
                DLOG_CRITICAL("pthread_sigmask failed: {}", err);
            }
        }
#endif
        left_ = true;
    }

  private:
    bool left_ = false;  // whether left region
#ifndef ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING
    bool mask_valid_ = false;
    sigset_t mask_;  // old mask of blocked signals
#endif
    low_level_alloc::arena *arena_;

    arena_lock(const arena_lock &) = delete;

    arena_lock &operator=(const arena_lock &) = delete;
};
}  // namespace

// create an appropriate magic number for an object at "ptr"
// "magic" should be kMagicAllocated or kMagicUnallocated
ABEL_FORCE_INLINE static uintptr_t Magic(uintptr_t magic, alloc_list::Header *ptr) {
    return magic ^ reinterpret_cast<uintptr_t>(ptr);
}

namespace {
size_t get_page_size() {
#ifdef _WIN32
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    return std::max(system_info.dwPageSize, system_info.dwAllocationGranularity);
#elif defined(__wasm__) || defined(__asmjs__)
    return getpagesize();
#else
    return sysconf(_SC_PAGESIZE);
#endif
}

size_t rounded_up_block_size() {
    // Round up block sizes to a power of two close to the header size.
    size_t round_up = 16;
    while (round_up < sizeof(alloc_list::Header)) {
        round_up += round_up;
    }
    return round_up;
}

}  // namespace

low_level_alloc::arena::arena(uint32_t flags_value)
        : mu(thread_internal::SCHEDULE_KERNEL_ONLY),
          allocation_count(0),
          flags(flags_value),
          pagesize(get_page_size()),
          round_up(rounded_up_block_size()),
          min_size(2 * round_up),
          random(0) {
    freelist.header.size = 0;
    freelist.header.magic =
            Magic(kMagicUnallocated, &freelist.header);
    freelist.header.arena = this;
    freelist.levels = 0;
    memset(freelist.next, 0, sizeof(freelist.next));
}

// L < meta_data_arena->mu
low_level_alloc::arena *low_level_alloc::new_arena(int32_t flags) {
    arena *meta_data_arena = default_arena();
#ifndef ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING
    if ((flags & low_level_alloc::kAsyncSignalSafe) != 0) {
        meta_data_arena = unhooked_async_sig_safe_arena();
    } else  // NOLINT(readability/braces)
#endif
    if ((flags & low_level_alloc::kCallMallocHook) == 0) {
        meta_data_arena = UnhookedArena();
    }
    arena *result =
            new(alloc_with_arena(sizeof(*result), meta_data_arena)) arena(flags);
    return result;
}

// L < arena->mu, L < arena->arena->mu
bool low_level_alloc::delete_arena(arena *arena) {
    DCHECK_MSG(
            arena != nullptr && arena != default_arena() && arena != UnhookedArena(),
            "may not delete default arena");
    arena_lock section(arena);
    if (arena->allocation_count != 0) {
        section.Leave();
        return false;
    }
    while (arena->freelist.next[0] != nullptr) {
        alloc_list *region = arena->freelist.next[0];
        size_t size = region->header.size;
        arena->freelist.next[0] = region->next[0];
        DCHECK_MSG(
                region->header.magic == Magic(kMagicUnallocated, &region->header),
                "bad magic number in DeleteArena()");
        DCHECK_MSG(region->header.arena == arena,
                   "bad arena pointer in DeleteArena()");
        DCHECK_MSG(size % arena->pagesize == 0,
                   "empty arena has non-page-aligned block size");
        DCHECK_MSG(reinterpret_cast<uintptr_t>(region) % arena->pagesize == 0,
                   "empty arena has non-page-aligned block");
        int munmap_result;
#ifdef _WIN32
        munmap_result = VirtualFree(region, 0, MEM_RELEASE);
        DCHECK(munmap_result != 0,
                       "low_level_alloc::DeleteArena: VitualFree failed");
#else
#ifndef ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING
        if ((arena->flags & low_level_alloc::kAsyncSignalSafe) == 0) {
            munmap_result = munmap(region, size);
        } else {
            munmap_result = base_internal::DirectMunmap(region, size);
        }
#else
        munmap_result = munmap(region, size);
#endif  // ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING
        if (munmap_result != 0) {
            DLOG_CRITICAL("low_level_alloc::DeleteArena: munmap failed: {}",
                          errno);
        }
#endif  // _WIN32
    }
    section.Leave();
    arena->~arena();
    free(arena);
    return true;
}

// ---------------------------------------------------------------------------

// Addition, checking for overflow.  The intent is to die if an external client
// manages to push through a request that would cause arithmetic to fail.
static ABEL_FORCE_INLINE uintptr_t checked_add(uintptr_t a, uintptr_t b) {
    uintptr_t sum = a + b;
    DCHECK_MSG(sum >= a, "low_level_alloc arithmetic overflow");
    return sum;
}

// Return value rounded up to next multiple of align.
// align must be a power of two.
static ABEL_FORCE_INLINE uintptr_t round_up(uintptr_t addr, uintptr_t align) {
    return checked_add(addr, align - 1) & ~(align - 1);
}

// Equivalent to "return prev->next[i]" but with sanity checking
// that the freelist is in the correct order, that it
// consists of regions marked "unallocated", and that no two regions
// are adjacent in memory (they should have been coalesced).
// L >= arena->mu
static alloc_list *Next(int i, alloc_list *prev, low_level_alloc::arena *arena) {
    DCHECK_MSG(i < prev->levels, "too few levels in Next()");
    alloc_list *next = prev->next[i];
    if (next != nullptr) {
        DCHECK_MSG(
                next->header.magic == Magic(kMagicUnallocated, &next->header),
                "bad magic number in Next()");
        DCHECK_MSG(next->header.arena == arena, "bad arena pointer in Next()");
        if (prev != &arena->freelist) {
            DCHECK_MSG(prev < next, "unordered freelist");
            DCHECK_MSG(reinterpret_cast<char *>(prev) + prev->header.size <
                       reinterpret_cast<char *>(next),
                       "malformed freelist");
        }
    }
    return next;
}

// coalesce list item "a" with its successor if they are adjacent.
static void coalesce(alloc_list *a) {
    alloc_list *n = a->next[0];
    if (n != nullptr && reinterpret_cast<char *>(a) + a->header.size ==
                        reinterpret_cast<char *>(n)) {
        low_level_alloc::arena *arena = a->header.arena;
        a->header.size += n->header.size;
        n->header.magic = 0;
        n->header.arena = nullptr;
        alloc_list *prev[kMaxLevel];
        LLA_SkiplistDelete(&arena->freelist, n, prev);
        LLA_SkiplistDelete(&arena->freelist, a, prev);
        a->levels = lla_skip_list_levels(a->header.size, arena->min_size,
                                         &arena->random);
        lla_skip_list_insert(&arena->freelist, a, prev);
    }
}

// Adds block at location "v" to the free list
// L >= arena->mu
static void AddToFreelist(void *v, low_level_alloc::arena *arena) {
    alloc_list *f = reinterpret_cast<alloc_list *>(
            reinterpret_cast<char *>(v) - sizeof(f->header));
    DCHECK_MSG(f->header.magic == Magic(kMagicAllocated, &f->header),
               "bad magic number in AddToFreelist()");
    DCHECK_MSG(f->header.arena == arena,
               "bad arena pointer in AddToFreelist()");
    f->levels = lla_skip_list_levels(f->header.size, arena->min_size,
                                     &arena->random);
    alloc_list *prev[kMaxLevel];
    lla_skip_list_insert(&arena->freelist, f, prev);
    f->header.magic = Magic(kMagicUnallocated, &f->header);
    coalesce(f);                  // maybe coalesce with successor
    coalesce(prev[0]);            // maybe coalesce with predecessor
}

// Frees storage allocated by low_level_alloc::Alloc().
// L < arena->mu
void low_level_alloc::free(void *v) {
    if (v != nullptr) {
        alloc_list *f = reinterpret_cast<alloc_list *>(
                reinterpret_cast<char *>(v) - sizeof(f->header));
        low_level_alloc::arena *arena = f->header.arena;
        arena_lock section(arena);
        AddToFreelist(v, arena);
        DCHECK_MSG(arena->allocation_count > 0, "nothing in arena to free");
        arena->allocation_count--;
        section.Leave();
    }
}

// allocates and returns a block of size bytes, to be freed with Free()
// L < arena->mu
static void *do_alloc_with_arena(size_t request, low_level_alloc::arena *arena) {
    void *result = nullptr;
    if (request != 0) {
        alloc_list *s;       // will point to region that satisfies request
        arena_lock section(arena);
        // round up with header
        size_t req_rnd = round_up(checked_add(request, sizeof(s->header)),
                                  arena->round_up);
        for (;;) {      // loop until we find a suitable region
            // find the minimum levels that a block of this size must have
            int i = lla_skip_list_levels(req_rnd, arena->min_size, nullptr) - 1;
            if (i < arena->freelist.levels) {   // potential blocks exist
                alloc_list *before = &arena->freelist;  // predecessor of s
                while ((s = Next(i, before, arena)) != nullptr &&
                       s->header.size < req_rnd) {
                    before = s;
                }
                if (s != nullptr) {       // we found a region
                    break;
                }
            }
            // we unlock before mmap() both because mmap() may call a callback hook,
            // and because it may be slow.
            arena->mu.unlock();
            // mmap generous 64K chunks to decrease
            // the chances/impact of fragmentation:
            size_t new_pages_size = round_up(req_rnd, arena->pagesize * 16);
            void *new_pages;
#ifdef _WIN32
            new_pages = VirtualAlloc(0, new_pages_size,
                                     MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            DCHECK_MSG(new_pages != nullptr, "VirtualAlloc failed");
#else
#ifndef ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING
            if ((arena->flags & low_level_alloc::kAsyncSignalSafe) != 0) {
                new_pages = base_internal::DirectMmap(nullptr, new_pages_size,
                                                      PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1,
                                                      0);
            } else {
                new_pages = mmap(nullptr, new_pages_size, PROT_WRITE | PROT_READ,
                                 MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
            }
#else
            new_pages = mmap(nullptr, new_pages_size, PROT_WRITE | PROT_READ,
                             MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
#endif  // ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING
            if (new_pages == MAP_FAILED) {
                DLOG_CRITICAL("mmap error: {}", errno);
            }

#endif  // _WIN32
            arena->mu.lock();
            s = reinterpret_cast<alloc_list *>(new_pages);
            s->header.size = new_pages_size;
            // Pretend the block is allocated; call AddToFreelist() to free it.
            s->header.magic = Magic(kMagicAllocated, &s->header);
            s->header.arena = arena;
            AddToFreelist(&s->levels, arena);  // insert new region into free list
        }
        alloc_list *prev[kMaxLevel];
        LLA_SkiplistDelete(&arena->freelist, s, prev);    // remove from free list
        // s points to the first free region that's big enough
        if (checked_add(req_rnd, arena->min_size) <= s->header.size) {
            // big enough to split
            alloc_list *n = reinterpret_cast<alloc_list *>
            (req_rnd + reinterpret_cast<char *>(s));
            n->header.size = s->header.size - req_rnd;
            n->header.magic = Magic(kMagicAllocated, &n->header);
            n->header.arena = arena;
            s->header.size = req_rnd;
            AddToFreelist(&n->levels, arena);
        }
        s->header.magic = Magic(kMagicAllocated, &s->header);
        DCHECK_MSG(s->header.arena == arena, "");
        arena->allocation_count++;
        section.Leave();
        result = &s->levels;
    }
    ANNOTATE_MEMORY_IS_UNINITIALIZED(result, request);
    return result;
}

void *low_level_alloc::alloc(size_t request) {
    void *result = do_alloc_with_arena(request, default_arena());
    return result;
}

void *low_level_alloc::alloc_with_arena(size_t request, arena *arena) {
    DCHECK_MSG(arena != nullptr, "must pass a valid arena");
    void *result = do_alloc_with_arena(request, arena);
    return result;
}

}  // namespace memory_internal

}  // namespace abel

#endif  // ABEL_LOW_LEVEL_ALLOC_MISSING
