//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


// Date: Fri Sep 10 13:34:25 CST 2010

#include <melon/utility/build_config.h>                        // OS_MACOSX
#include <errno.h>                                     // errno
#include <string.h>                                    // strerror_r
#include <stdlib.h>                                    // EXIT_FAILURE
#include <stdio.h>                                     // snprintf
#include <pthread.h>                                   // pthread_mutex_t
#include <unistd.h>                                    // _exit
#include <melon/utility/scoped_lock.h>                         // MELON_SCOPED_LOCK

namespace mutil {

const int ERRNO_BEGIN = -32768;
const int ERRNO_END = 32768;
static const char* errno_desc[ERRNO_END - ERRNO_BEGIN] = {};
static pthread_mutex_t modify_desc_mutex = PTHREAD_MUTEX_INITIALIZER;

const size_t ERROR_BUFSIZE = 64;
__thread char tls_error_buf[ERROR_BUFSIZE];

int DescribeCustomizedErrno(
    int error_code, const char* error_name, const char* description) {
    MELON_SCOPED_LOCK(modify_desc_mutex);
    if (error_code < ERRNO_BEGIN || error_code >= ERRNO_END) {
        // error() is a non-portable GNU extension that should not be used.
        fprintf(stderr, "Fail to define %s(%d) which is out of range, abort.",
              error_name, error_code);
        _exit(1);
    }
    const char* desc = errno_desc[error_code - ERRNO_BEGIN];
    if (desc) {
        if (strcmp(desc, description) == 0) {
            fprintf(stderr, "WARNING: Detected shared library loading\n");
            return -1;
        }
    } else {
#if defined(OS_MACOSX)
        const int rc = strerror_r(error_code, tls_error_buf, ERROR_BUFSIZE);
        if (rc != EINVAL)
#else
        desc = strerror_r(error_code, tls_error_buf, ERROR_BUFSIZE);
        if (desc && strncmp(desc, "Unknown error", 13) != 0)
#endif
        {
            fprintf(stderr, "WARNING: Fail to define %s(%d) which is already defined as `%s'",
                    error_name, error_code, desc);
        }
    }
    errno_desc[error_code - ERRNO_BEGIN] = description;
    return 0;  // must
}

}  // namespace mutil

const char* berror(int error_code) {
    if (error_code == -1) {
        return "General error -1";
    }
    if (error_code >= mutil::ERRNO_BEGIN && error_code < mutil::ERRNO_END) {
        const char* s = mutil::errno_desc[error_code - mutil::ERRNO_BEGIN];
        if (s) {
            return s;
        }
#if defined(OS_MACOSX)
        const int rc = strerror_r(error_code, mutil::tls_error_buf, mutil::ERROR_BUFSIZE);
        if (rc == 0 || rc == ERANGE/*bufsize is not long enough*/) {
            return mutil::tls_error_buf;
        }
#else
        s = strerror_r(error_code, mutil::tls_error_buf, mutil::ERROR_BUFSIZE);
        if (s) {
            return s;
        }
#endif
    }
    snprintf(mutil::tls_error_buf, mutil::ERROR_BUFSIZE,
             "Unknown error %d", error_code);
    return mutil::tls_error_buf;
}

const char* berror() {
    return berror(errno);
}
