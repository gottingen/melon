
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_LOG_SERVERITY_H_
#define MELON_LOG_SERVERITY_H_

#include "melon/base/profile.h"
// Variables of type log_severity are widely taken to lie in the range
// [0, NUM_SEVERITIES-1].  Be careful to preserve this assumption if
// you ever need to change their values or add a new severity.
namespace melon::log {
    typedef int log_severity;

    const int MELON_TRACE = 0, MELON_DEBUG = 1, MELON_INFO = 2, MELON_WARNING = 3, MELON_ERROR = 4, MELON_FATAL = 5,
            NUM_SEVERITIES = 6;
#ifndef MELON_NO_ABBREVIATED_SEVERITIES
# ifdef ERROR
#  error ERROR macro is defined. Define MELON_LOG_NO_ABBREVIATED_SEVERITIES before including logging.h. See the document for detail.
# endif
    const int TRACE = melon::log::MELON_TRACE, DEBUG = melon::log::MELON_DEBUG, INFO = MELON_INFO, WARNING = melon::log::MELON_WARNING,
            ERROR = melon::log::MELON_ERROR, FATAL = melon::log::MELON_FATAL;
#endif

// DFATAL is FATAL in debug mode, ERROR in normal mode
#ifdef NDEBUG
#define DFATAL_LEVEL ERROR
#else
#define DFATAL_LEVEL FATAL
#endif

    extern MELON_EXPORT const char *const log_severity_names[NUM_SEVERITIES];

// NDEBUG usage helpers related to (RAW_)MELON_DCHECK:
//
// DEBUG_MODE is for small !NDEBUG uses like
//   if (DEBUG_MODE) foo.CheckThatFoo();
// instead of substantially more verbose
//   #ifndef NDEBUG
//     foo.CheckThatFoo();
//   #endif
//
// MELON_IF_DEBUG_MODE is for small !NDEBUG uses like
//   MELON_IF_DEBUG_MODE( string error; )
//   MELON_DCHECK(Foo(&error)) << error;
// instead of substantially more verbose
//   #ifndef NDEBUG
//     string error;
//     MELON_DCHECK(Foo(&error)) << error;
//   #endif
//
#ifdef NDEBUG
    enum {
        DEBUG_MODE = 0
    };
#define MELON_IF_DEBUG_MODE(x)
#else
    enum { DEBUG_MODE = 1 };
#define MELON_IF_DEBUG_MODE(x) x
#endif
}

#endif // MELON_LOG_SERVERITY_H_
