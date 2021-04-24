// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_SYSTEM_PROCESS_H_
#define ABEL_SYSTEM_PROCESS_H_

#include "abel/system/platform_headers.h"

namespace abel {

inline int pid() {

#ifdef _WIN32
    return static_cast<int>(::GetCurrentProcessId());
#else
    return static_cast<int>(::getpid());
#endif
}
}
#endif  // ABEL_SYSTEM_PROCESS_H_
