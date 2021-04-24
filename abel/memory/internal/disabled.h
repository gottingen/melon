//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_MEMORY_DISABLE_H_
#define ABEL_MEMORY_DISABLE_H_

#include "abel/memory/internal/type_descriptor.h"

namespace abel {
    namespace memory_internal {

        void *disabled_get(const type_descriptor &desc);

        void disabled_put(const type_descriptor &desc, void *ptr);
    }  // namespace memory_internal
}
#endif  // ABEL_MEMORY_DISABLE_H_
