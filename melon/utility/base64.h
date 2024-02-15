// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MUTIL_BASE64_H__
#define MUTIL_BASE64_H__

#include <string>

#include "melon/utility/base_export.h"
#include "melon/utility/strings/string_piece.h"

namespace mutil {

// Encodes the input string in base64.
MUTIL_EXPORT void Base64Encode(const StringPiece& input, std::string* output);

// Decodes the base64 input string.  Returns true if successful and false
// otherwise.  The output string is only modified if successful.
MUTIL_EXPORT bool Base64Decode(const StringPiece& input, std::string* output);

}  // namespace mutil

#endif  // MUTIL_BASE64_H__
