// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MUTIL_DEBUG_DUMP_WITHOUT_CRASHING_H_
#define MUTIL_DEBUG_DUMP_WITHOUT_CRASHING_H_

#include "melon/utility/base_export.h"
#include "melon/utility/compiler_specific.h"
#include "melon/utility/build_config.h"

namespace mutil {

namespace debug {

// Handler to silently dump the current process without crashing.
MUTIL_EXPORT void DumpWithoutCrashing();

// Sets a function that'll be invoked to dump the current process when
// DumpWithoutCrashing() is called.
MUTIL_EXPORT void SetDumpWithoutCrashingFunction(void (CDECL *function)());

}  // namespace debug

}  // namespace mutil

#endif  // MUTIL_DEBUG_DUMP_WITHOUT_CRASHING_H_
