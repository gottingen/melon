// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Defines some functions that intentionally do an invalid memory access in
// order to trigger an AddressSanitizer (ASan) error report.

#ifndef MUTIL_DEBUG_ASAN_INVALID_ACCESS_H_
#define MUTIL_DEBUG_ASAN_INVALID_ACCESS_H_

#include "melon/utility/base_export.h"
#include <melon/utility/compiler_specific.h>

namespace mutil {
namespace debug {

#if defined(ADDRESS_SANITIZER) || defined(SYZYASAN)

// Generates an heap buffer overflow.
MUTIL_EXPORT NOINLINE void AsanHeapOverflow();

// Generates an heap buffer underflow.
MUTIL_EXPORT NOINLINE void AsanHeapUnderflow();

// Generates an use after free.
MUTIL_EXPORT NOINLINE void AsanHeapUseAfterFree();

#endif  // ADDRESS_SANITIZER || SYZYASAN

// The "corrupt-block" and "corrupt-heap" classes of bugs is specific to
// SyzyASan.
#if defined(SYZYASAN)

// Corrupts a memory block and makes sure that the corruption gets detected when
// we try to free this block.
MUTIL_EXPORT NOINLINE void AsanCorruptHeapBlock();

// Corrupts the heap and makes sure that the corruption gets detected when a
// crash occur.
MUTIL_EXPORT NOINLINE void AsanCorruptHeap();

#endif  // SYZYASAN

}  // namespace debug
}  // namespace mutil

#endif  // MUTIL_DEBUG_ASAN_INVALID_ACCESS_H_
