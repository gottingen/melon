// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MUTIL_GUID_H_
#define MUTIL_GUID_H_

#include <string>

#include "melon/utility/base_export.h"
#include "melon/utility/basictypes.h"
#include "melon/utility/build_config.h"

namespace mutil {

// Generate a 128-bit random GUID of the form: "%08X-%04X-%04X-%04X-%012llX".
// If GUID generation fails an empty string is returned.
// The POSIX implementation uses psuedo random number generation to create
// the GUID.  The Windows implementation uses system services.
MUTIL_EXPORT std::string GenerateGUID();

// Returns true if the input string conforms to the GUID format.
MUTIL_EXPORT bool IsValidGUID(const std::string& guid);

#if defined(OS_POSIX)
// For unit testing purposes only.  Do not use outside of tests.
MUTIL_EXPORT std::string RandomDataToGUIDString(const uint64_t bytes[2]);
#endif

}  // namespace mutil

#endif  // MUTIL_GUID_H_
