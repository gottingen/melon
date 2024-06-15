// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MUTIL_STRINGS_STRINGPRINTF_H_
#define MUTIL_STRINGS_STRINGPRINTF_H_

#include <stdarg.h>   // va_list

#include <string>

#include <melon/base/base_export.h>
#include <melon/base/compiler_specific.h>

namespace mutil {

// Return a C++ string given printf-like input.
MUTIL_EXPORT std::string StringPrintf(const char* format, ...)
    PRINTF_FORMAT(1, 2);
// OS_ANDROID's libc does not support wchar_t, so several overloads are omitted.
#if !defined(OS_ANDROID)
MUTIL_EXPORT std::wstring StringPrintf(const wchar_t* format, ...)
    WPRINTF_FORMAT(1, 2);
#endif

// Return a C++ string given vprintf-like input.
MUTIL_EXPORT std::string StringPrintV(const char* format, va_list ap)
    PRINTF_FORMAT(1, 0);

// Store result into a supplied string and return it.
MUTIL_EXPORT const std::string& SStringPrintf(std::string* dst,
                                             const char* format, ...)
    PRINTF_FORMAT(2, 3);
#if !defined(OS_ANDROID)
MUTIL_EXPORT const std::wstring& SStringPrintf(std::wstring* dst,
                                              const wchar_t* format, ...)
    WPRINTF_FORMAT(2, 3);
#endif

// Append result to a supplied string.
MUTIL_EXPORT void StringAppendF(std::string* dst, const char* format, ...)
    PRINTF_FORMAT(2, 3);
#if !defined(OS_ANDROID)
// TODO(evanm): this is only used in a few places in the code;
// replace with string16 version.
MUTIL_EXPORT void StringAppendF(std::wstring* dst, const wchar_t* format, ...)
    WPRINTF_FORMAT(2, 3);
#endif

// Lower-level routine that takes a va_list and appends to a specified
// string.  All other routines are just convenience wrappers around it.
MUTIL_EXPORT void StringAppendV(std::string* dst, const char* format, va_list ap)
    PRINTF_FORMAT(2, 0);
#if !defined(OS_ANDROID)
MUTIL_EXPORT void StringAppendV(std::wstring* dst,
                               const wchar_t* format, va_list ap)
    WPRINTF_FORMAT(2, 0);
#endif

}  // namespace mutil

#endif  // MUTIL_STRINGS_STRINGPRINTF_H_
