//
// Created by liyinbin on 2021/4/5.
//

#ifndef ABEL_FIBER_INDEX_ALLOC_H_
#define ABEL_FIBER_INDEX_ALLOC_H_


#include <cstddef>
#include <mutex>
#include <vector>

#include "abel/memory/non_destroy.h"

namespace abel {

    // This class helps you allocate indices. Indices are numbered from 0.
    //
    // Note that this class does NOT perform well. It's not intended for use in
    // scenarios where efficient index allocation is needed.
    class index_alloc {
    public:
        // To prevent interference between index allocation for different purpose, you
        // can use tag type to separate different allocations.
        template <class Tag>
        static index_alloc* For() {
            static non_destroyed_singleton<index_alloc> ia;
            return ia.get();
        }

        // Get next available index. If there's previously freed index (via `Free`),
        // it's returned, otherwise a new one is allocated.
        std::size_t next();

        // Free an index. It can be reused later.
        void free(std::size_t index);

    private:
        friend class abel::non_destroyed_singleton<index_alloc>;
        // You shouldn't instantiate this class yourself, call `For<...>()` instead.
        index_alloc() = default;
        ~index_alloc() = default;

        std::mutex lock_;
        std::size_t current_{};
        std::vector<std::size_t> recycled_;
    };

}  // namespace abel

#endif  // ABEL_FIBER_INDEX_ALLOC_H_
