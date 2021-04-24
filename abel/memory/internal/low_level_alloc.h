// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//

#ifndef ABEL_MEMORY_INTERNAL_LOW_LEVEL_ALLOC_H_
#define ABEL_MEMORY_INTERNAL_LOW_LEVEL_ALLOC_H_

// A simple thread-safe memory allocator that does not depend on
// mutexes or thread-specific data.  It is intended to be used
// sparingly, and only when malloc() would introduce an unwanted
// dependency, such as inside the heap-checker, or the mutex
// implementation.

#include <sys/types.h>
#include <cstdint>
#include "abel/base/profile.h"

// low_level_alloc requires that the platform support low-level
// allocation of virtual memory. Platforms lacking this cannot use
// low_level_alloc.
#ifdef ABEL_LOW_LEVEL_ALLOC_MISSING
#error ABEL_LOW_LEVEL_ALLOC_MISSING cannot be directly set
#elif !defined(ABEL_HAVE_MMAP) && !defined(_WIN32)
#define ABEL_LOW_LEVEL_ALLOC_MISSING 1
#endif

// Using low_level_alloc with kAsyncSignalSafe isn't supported on Windows or
// asm.js / WebAssembly.
// See https://kripken.github.io/emscripten-site/docs/porting/pthreads.html
// for more information.
#ifdef ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING
#error ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING cannot be directly set
#elif defined(_WIN32) || defined(__asmjs__) || defined(__wasm__)
#define ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING 1
#endif

#include <cstddef>

namespace abel {

namespace memory_internal {

class low_level_alloc {
  public:
    struct arena;       // an arena from which memory may be allocated

    // Returns a pointer to a block of at least "request" bytes
    // that have been newly allocated from the specific arena.
    // for alloc() call the default_arena() is used.
    // Returns 0 if passed request==0.
    // Does not return 0 under other circumstances; it crashes if memory
    // is not available.
    static void *alloc(size_t request) ABEL_ATTRIBUTE_SECTION(malloc_hook);

    static void *alloc_with_arena(size_t request, arena *arena)
    ABEL_ATTRIBUTE_SECTION(malloc_hook);

    // Deallocates a region of memory that was previously allocated with
    // Alloc().   Does nothing if passed 0.   "s" must be either 0,
    // or must have been returned from a call to Alloc() and not yet passed to
    // Free() since that call to Alloc().  The space is returned to the arena
    // from which it was allocated.
    static void free(void *s) ABEL_ATTRIBUTE_SECTION(malloc_hook);

    // ABEL_ATTRIBUTE_SECTION(malloc_hook) for Alloc* and Free
    // are to put all callers of MallocHook::Invoke* in this module
    // into special section,
    // so that MallocHook::GetCallerStackTrace can function accurately.

    // Create a new arena.
    // The root metadata for the new arena is allocated in the
    // meta_data_arena; the DefaultArena() can be passed for meta_data_arena.
    // These values may be ored into flags:
    enum {
        // Report calls to Alloc() and Free() via the MallocHook interface.
        // Set in the DefaultArena.
        kCallMallocHook = 0x0001,

#ifndef ABEL_LOW_LEVEL_ALLOC_ASYNC_SIGNAL_SAFE_MISSING
        // Make calls to Alloc(), Free() be async-signal-safe. Not set in
        // DefaultArena(). Not supported on all platforms.
        kAsyncSignalSafe = 0x0002,
#endif
    };

    // Construct a new arena.  The allocation of the underlying metadata honors
    // the provided flags.  For example, the call new_arena(kAsyncSignalSafe)
    // is itself async-signal-safe, as well as generatating an arena that provides
    // async-signal-safe Alloc/Free.
    static arena *new_arena(int32_t flags);

    // Destroys an arena allocated by new_arena and returns true,
    // provided no allocated blocks remain in the arena.
    // If allocated blocks remain in the arena, does nothing and
    // returns false.
    // It is illegal to attempt to destroy the DefaultArena().
    static bool delete_arena(arena *arena);

    // The default arena that always exists.
    static arena *default_arena();

  private:
    low_level_alloc();      // no instances
};

}  // namespace memory_internal

}  // namespace abel

#endif  // ABEL_MEMORY_INTERNAL_LOW_LEVEL_ALLOC_H_
