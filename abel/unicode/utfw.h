//
// Created by 李寅斌 on 2021/2/18.
//

#ifndef ABEL_UNICODE_UTFW_H_
#define ABEL_UNICODE_UTFW_H_


#include "abel/unicode/utf16.h"
#include "abel/unicode/utf32.h"


namespace abel {

#if defined(_WIN32)
using utfw = utf16;
#elif defined(__linux__) || defined(__APPLE__)
using utfw = utf32;
#else
#error Unsupported platform
#endif


}  // namespace abel

#endif  // ABEL_UNICODE_UTFW_H_
