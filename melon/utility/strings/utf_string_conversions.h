// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MUTIL_STRINGS_UTF_STRING_CONVERSIONS_H_
#define MUTIL_STRINGS_UTF_STRING_CONVERSIONS_H_

#include <string>

#include "melon/utility/base_export.h"
#include "melon/utility/strings/string16.h"
#include "melon/utility/strings/string_piece.h"

namespace mutil {

// These convert between UTF-8, -16, and -32 strings. They are potentially slow,
// so avoid unnecessary conversions. The low-level versions return a boolean
// indicating whether the conversion was 100% valid. In this case, it will still
// do the best it can and put the result in the output buffer. The versions that
// return strings ignore this error and just return the best conversion
// possible.
MUTIL_EXPORT bool WideToUTF8(const wchar_t* src, size_t src_len,
                            std::string* output);
MUTIL_EXPORT std::string WideToUTF8(const std::wstring& wide);
MUTIL_EXPORT bool UTF8ToWide(const char* src, size_t src_len,
                            std::wstring* output);
MUTIL_EXPORT std::wstring UTF8ToWide(const StringPiece& utf8);

MUTIL_EXPORT bool WideToUTF16(const wchar_t* src, size_t src_len,
                             string16* output);
MUTIL_EXPORT string16 WideToUTF16(const std::wstring& wide);
MUTIL_EXPORT bool UTF16ToWide(const char16* src, size_t src_len,
                             std::wstring* output);
MUTIL_EXPORT std::wstring UTF16ToWide(const string16& utf16);

MUTIL_EXPORT bool UTF8ToUTF16(const char* src, size_t src_len, string16* output);
MUTIL_EXPORT string16 UTF8ToUTF16(const StringPiece& utf8);
MUTIL_EXPORT bool UTF16ToUTF8(const char16* src, size_t src_len,
                             std::string* output);
MUTIL_EXPORT std::string UTF16ToUTF8(const string16& utf16);

// These convert an ASCII string, typically a hardcoded constant, to a
// UTF16/Wide string.
MUTIL_EXPORT std::wstring ASCIIToWide(const StringPiece& ascii);
MUTIL_EXPORT string16 ASCIIToUTF16(const StringPiece& ascii);

// Converts to 7-bit ASCII by truncating. The result must be known to be ASCII
// beforehand.
MUTIL_EXPORT std::string UTF16ToASCII(const string16& utf16);

}  // namespace mutil

#endif  // MUTIL_STRINGS_UTF_STRING_CONVERSIONS_H_
