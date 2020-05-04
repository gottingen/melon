//
// Created by liyinbin on 2020/4/7.
//

#include <abel/base/profile.h>
#include <errno.h>                                     // errno
#include <string.h>                                    // strerror_r
#include <stdlib.h>                                    // EXIT_FAILURE
#include <stdio.h>                                     // snprintf
#include <pthread.h>                                   // pthread_mutex_t
#include <unistd.h>// _exit
#include <abel/thread/mutex.h>
#include <abel/base/profile.h>
#include <abel/system/error_code.h>

namespace abel {


    const int ERRNO_BEGIN = -32768;
    const int ERRNO_END = 32768;
    static const char *errno_desc[ERRNO_END - ERRNO_BEGIN] = {};
    static abel::mutex modify_desc_mutex(abel::kConstInit);

    const size_t ERROR_BUFSIZE = 64;
    static ABEL_THREAD_LOCAL char tls_error_buf[ERROR_BUFSIZE];

    int describe_customized_errno(
            int error_code, const char *error_name, const char *description) {
        abel::mutex_lock lk(&modify_desc_mutex);
        if (error_code < ERRNO_BEGIN || error_code >= ERRNO_END) {
            // error() is a non-portable GNU extension that should not be used.
            fprintf(stderr, "Fail to define %s(%d) which is out of range, abort.",
                    error_name, error_code);
            _exit(1);
        }
        const char *desc = errno_desc[error_code - ERRNO_BEGIN];
        if (desc) {
            if (strcmp(desc, description) == 0) {
                fprintf(stderr, "WARNING: Detected shared library loading\n");
                return -1;
            }
        } else {
#if defined(ABEL_PLATFORM_APPLE)
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


    const char *abel_error(int error_code) {
        if (error_code == -1) {
            return "General error -1";
        }
        if (error_code >= ERRNO_BEGIN && error_code < ERRNO_END) {
            const char *s = errno_desc[error_code - ERRNO_BEGIN];
            if (s) {
                return s;
            }
#if defined(ABEL_PLATFORM_APPLE)
            const int rc = strerror_r(error_code, tls_error_buf, ERROR_BUFSIZE);
            if (rc == 0 || rc == ERANGE/*bufsize is not long enough*/) {
                return tls_error_buf;
            }
#else
            s = strerror_r(error_code, tls_error_buf, ERROR_BUFSIZE);
        if (s) {
            return s;
        }
#endif
        }
        ::snprintf(tls_error_buf, ERROR_BUFSIZE,
                   "Unknown error %d", error_code);
        return tls_error_buf;
    }

    const char *abel_error() {
        return abel_error(errno);
    }
} //namespace abel
