
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "melon/base/profile.h"
#include <errno.h>                                     // errno
#include <string.h>                                    // strerror_r
#include <cstdlib>                                    // EXIT_FAILURE
#include <stdio.h>                                     // snprintf
#include <pthread.h>                                   // pthread_mutex_t
#include <unistd.h>                                    // _exit
#include "melon/base/scoped_lock.h"                         // MELON_SCOPED_LOCK

namespace melon::base {

const int ERRNO_BEGIN = -32768;
const int ERRNO_END = 32768;
static const char* errno_desc[ERRNO_END - ERRNO_BEGIN] = {};
static pthread_mutex_t modify_desc_mutex = PTHREAD_MUTEX_INITIALIZER;

const size_t ERROR_BUFSIZE = 64;
__thread char tls_error_buf[ERROR_BUFSIZE];

int describe_customized_errno(
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
#if defined(MELON_PLATFORM_OSX)
        const int rc = strerror_r(error_code, tls_error_buf, ERROR_BUFSIZE);
        if (rc != EINVAL)
#else
        desc = strerror_r(error_code, tls_error_buf, ERROR_BUFSIZE);
        if (desc && strncmp(desc, "Unknown error", 13) != 0)
#endif
        {
            fprintf(stderr, "Fail to define %s(%d) which is already defined as `%s', abort.",
                    error_name, error_code, desc);
            _exit(1);
        }
    }
    errno_desc[error_code - ERRNO_BEGIN] = description;
    return 0;  // must
}

}  // namespace melon::base

const char* melon_error(int error_code) {
    if (error_code == -1) {
        return "General error -1";
    }
    if (error_code >= melon::base::ERRNO_BEGIN && error_code < melon::base::ERRNO_END) {
        const char* s = melon::base::errno_desc[error_code - melon::base::ERRNO_BEGIN];
        if (s) {
            return s;
        }
#if defined(MELON_PLATFORM_OSX)
        const int rc = strerror_r(error_code, melon::base::tls_error_buf, melon::base::ERROR_BUFSIZE);
        if (rc == 0 || rc == ERANGE/*bufsize is not long enough*/) {
            return melon::base::tls_error_buf;
        }
#else
        s = strerror_r(error_code, melon::base::tls_error_buf, melon::base::ERROR_BUFSIZE);
        if (s) {
            return s;
        }
#endif
    }
    snprintf(melon::base::tls_error_buf, melon::base::ERROR_BUFSIZE,
             "Unknown error %d", error_code);
    return melon::base::tls_error_buf;
}

const char* melon_error() {
    return melon_error(errno);
}
