//
// Created by liyinbin on 2021/4/3.
//

#include "abel/memory/internal/global.h"
#include "abel/log/logging.h"

namespace abel {

    namespace memory_internal {
        void *global_get(const type_descriptor &desc) {
            DCHECK_MSG(false, "not implement");
        }

        void global_put(const type_descriptor &desc, void *ptr) {
            DCHECK_MSG(false, "not implement");
        }
    }  // namespace memory_internal
}  // namespace abel
