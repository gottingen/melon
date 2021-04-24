// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_SYSTEM_FD_UTIL_H_
#define ABEL_SYSTEM_FD_UTIL_H_

#include "abel/system/platform_headers.h"
#include <exception>

namespace abel {

inline bool prevent_child_fd(FILE *f) {

#ifdef _WIN32
#if !defined(__cplusplus_winrt)
    auto file_handle = (HANDLE)_get_osfhandle(_fileno(f));
    if (!::SetHandleInformation(file_handle, HANDLE_FLAG_INHERIT, 0))
        return false;
#endif
#else
    auto fd = fileno(f);
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        return false;
    }
#endif
    return true;
}

}
#endif  // ABEL_SYSTEM_FD_UTIL_H_
