//
// Created by liyinbin on 2021/4/3.
//

#include "abel/memory/object_pool.h"

namespace abel {

    namespace memory_internal {

        void InitializeObjectPoolForCurrentThread() {
            // Only `MemoryNodeShared` support this for now.
           // detail::memory_node_shared::EarlyInitializeForCurrentThread();
        }

    }  // namespace memory_internal

}  // namespace abel
