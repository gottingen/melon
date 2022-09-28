
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#pragma once

#if defined(__linux__) || defined(__APPLE__)

#include "melon/strings/utf/cp_utf32.h"

namespace melon {
    using utfw = utf32;
}

#else
#error Unsupported platform
#endif
