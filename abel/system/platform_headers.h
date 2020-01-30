//
// Created by liyinbin on 2020/1/30.
//

#ifndef ABEL_SYSTEM_PLATFORM_HEADERS_H_
#define ABEL_SYSTEM_PLATFORM_HEADERS_H_


#include <abel/base/profile.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX // prevent windows redefining min/max
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <io.h>      // _get_osfhandle and _isatty support
#include <process.h> //  _get_pid support
#include <windows.h>

#ifdef __MINGW32__
#include <share.h>
#endif

#else // unix

#include <fcntl.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/syscall.h> //Use gettid() syscall under linux to get thread id

#elif __FreeBSD__
#include <sys/thr.h> //Use thr_self() syscall under FreeBSD to get thread id
#endif

#endif // unix


#endif //ABEL_SYSTEM_PLATFORM_HEADERS_H_
