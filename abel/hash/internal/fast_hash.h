// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_HASH_INTERNAL_FAST_HASH_H_
#define ABEL_HASH_INTERNAL_FAST_HASH_H_

#include <string_view>
#include <cstdint>
#include <cstddef>

namespace abel {

namespace hash_internal {
uint32_t fast_hash(const char *src, size_t len);

}  // namespace hash_internal
}  // namespace abel

#endif  // ABEL_HASH_INTERNAL_FAST_HASH_H_
