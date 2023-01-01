// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/memory/aligned_memory.h"
#include "melon/log/logging.h"

#if defined(MELON_PLATFORM_ANDROID)
#include <malloc.h>
#endif

namespace melon {

    void *aligned_alloc(size_t size, size_t alignment) {
        MELON_DCHECK_GT(size, 0U);
        MELON_DCHECK_EQ(alignment & (alignment - 1), 0U);
        MELON_DCHECK_EQ(alignment % sizeof(void *), 0U);
        void *ptr = nullptr;
#if defined(MELON_COMPILER_MSVC)
        ptr = _aligned_malloc(size, alignment);
      // Android technically supports posix_memalign(), but does not expose it in
      // the current version of the library headers used by Chrome.  Luckily,
      // memalign() on Android returns pointers which can safely be used with
      // free(), so we can use it instead.  Issue filed to document this:
      // http://code.google.com/p/android/issues/detail?id=35391
#elif defined(MELON_PLATFORM_ANDROID)
        ptr = memalign(alignment, size);
#else
        if (posix_memalign(&ptr, alignment, size))
            ptr = nullptr;
#endif
        // Since aligned allocations may fail for non-memory related reasons, force a
        // crash if we encounter a failed allocation; maintaining consistent behavior
        // with a normal allocation failure in Chrome.
        if (!ptr) {
            MELON_DLOG(ERROR) << "If you crashed here, your aligned allocation is incorrect: "
                        << "size=" << size << ", alignment=" << alignment;
            MELON_CHECK(false);
        }
        // Sanity check alignment just to be safe.
        MELON_DCHECK_EQ(reinterpret_cast<uintptr_t>(ptr) & (alignment - 1), 0U);
        return ptr;
    }

}  // namespace melon
