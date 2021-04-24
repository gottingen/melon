//
// Created by liyinbin on 2021/4/3.
//

#ifndef ABEL_MEMORY_GLOBAL_H_
#define ABEL_MEMORY_GLOBAL_H_

#include "abel/memory/internal/type_descriptor.h"

namespace abel {

    namespace memory_internal {
        void* global_get(const type_descriptor& desc);
        void global_put(const type_descriptor& desc, void* ptr);
    }  // namespace memory_internal
}  // namespace abel

#endif  // ABEL_MEMORY_GLOBAL_H_
