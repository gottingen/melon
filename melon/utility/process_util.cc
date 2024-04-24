// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


// Process-related Info

// Date: Wed Apr 11 14:35:56 CST 2018

#include <fcntl.h>                      // open
#include <stdio.h>                      // snprintf
#include <sys/types.h>  
#include <sys/uio.h>
#include <unistd.h>                     // read, gitpid
#include <sstream>                      // std::ostringstream
#include "melon/utility/fd_guard.h"             // mutil::fd_guard
#include "melon/utility/logging.h"
#include "melon/utility/popen.h"                // read_command_output
#include "melon/utility/process_util.h"

namespace mutil {

ssize_t ReadCommandLine(char* buf, size_t len, bool with_args) {
#if defined(OS_LINUX)
    mutil::fd_guard fd(open("/proc/self/cmdline", O_RDONLY));
    if (fd < 0) {
        LOG(ERROR) << "Fail to open /proc/self/cmdline";
        return -1;
    }
    ssize_t nr = read(fd, buf, len);
    if (nr <= 0) {
        LOG(ERROR) << "Fail to read /proc/self/cmdline";
        return -1;
    }
#elif defined(OS_MACOSX)
    static pid_t pid = getpid();
    std::ostringstream oss;
    char cmdbuf[32];
    snprintf(cmdbuf, sizeof(cmdbuf), "ps -p %ld -o command=", (long)pid);
    if (mutil::read_command_output(oss, cmdbuf) != 0) {
        LOG(ERROR) << "Fail to read cmdline";
        return -1;
    }
    const std::string& result = oss.str();
    ssize_t nr = std::min(result.size(), len);
    memcpy(buf, result.data(), nr);
#else
    #error Not Implemented
#endif

    if (with_args) {
        if ((size_t)nr == len) {
            return len;
        }
        for (ssize_t i = 0; i < nr; ++i) {
            if (buf[i] == '\0') {
                buf[i] = '\n';
            }
        }
        return nr;
    } else {
        for (ssize_t i = 0; i < nr; ++i) {
            // The command in macos is separated with space and ended with '\n'
            if (buf[i] == '\0' || buf[i] == '\n' || buf[i] == ' ') {
                return i;
            }
        }
        if ((size_t)nr == len) {
            LOG(ERROR) << "buf is not big enough";
            return -1;
        }
        return nr;
    }
}

} // namespace mutil
