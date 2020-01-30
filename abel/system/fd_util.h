//
// Created by liyinbin on 2020/1/30.
//

#ifndef ABEL_SYSTEM_FD_UTIL_H_
#define ABEL_SYSTEM_FD_UTIL_H_

#include <abel/system/platform_headers.h>
namespace abel {

inline void prevent_child_fd (FILE *f) {

#ifdef _WIN32
    #if !defined(__cplusplus_winrt)
    auto file_handle = (HANDLE)_get_osfhandle(_fileno(f));
    if (!::SetHandleInformation(file_handle, HANDLE_FLAG_INHERIT, 0))
        throw spdlog_ex("SetHandleInformation failed", errno);
    #endif
#else
    auto fd = fileno(f);
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        throw spdlog_ex("fcntl with FD_CLOEXEC failed", errno);
    }
#endif
}

}
#endif //ABEL_SYSTEM_FD_UTIL_H_
