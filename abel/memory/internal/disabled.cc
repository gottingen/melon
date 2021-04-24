//
// Created by liyinbin on 2021/4/3.
//

#include "abel/memory/internal/disabled.h"

namespace abel {

    namespace memory_internal {

        void *disabled_get(const type_descriptor &desc) {
            auto rc = desc.create();
            return rc;
        }

        void disabled_put(const type_descriptor &desc, void *ptr) { desc.destroy(ptr); }

    }  // namespace memory_internal
}  // namespace abel
