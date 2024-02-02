// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// Date: 2012-10-08 23:53:50

// Merged chromium log and streaming log.

#ifndef MELON_COMMON_LOGGING_H_
#define MELON_COMMON_LOGGING_H_

#include <inttypes.h>
#include <string>
#include <cstring>
#include <sstream>
#include "melon/butil/macros.h"    // BAIDU_CONCAT
#include "melon/butil/atomicops.h" // Used by LOG_EVERY_N, LOG_FIRST_N etc
#include "melon/butil/time.h"      // gettimeofday_us()

#define GLOG_USE_GLOG_EXPORT 1
# include <glog/logging.h>
# include <glog/raw_logging.h>
// define macros that not implemented in glog
# ifndef DCHECK_IS_ON   // glog didn't define DCHECK_IS_ON in older version
#  if defined(NDEBUG)
#    define DCHECK_IS_ON() 0
#  else
#    define DCHECK_IS_ON() 1
#  endif  // NDEBUG
# endif // DCHECK_IS_ON

# if DCHECK_IS_ON() 
#  define DPLOG(...) PLOG(__VA_ARGS__)
#  define DPLOG_IF(...) PLOG_IF(__VA_ARGS__)
#  define DPCHECK(...) PCHECK(__VA_ARGS__)
#  define DVPLOG(...) VLOG(__VA_ARGS__)
# else 
#  define DPLOG(...) DLOG(__VA_ARGS__)
#  define DPLOG_IF(...) DLOG_IF(__VA_ARGS__)
#  define DPCHECK(...) DCHECK(__VA_ARGS__)
#  define DVPLOG(...) DVLOG(__VA_ARGS__)
# endif

#define LOG_AT(severity, file, line)                                    \
    google::LogMessage(file, line, google::severity).stream()

#ifndef NOTIMPLEMENTED_POLICY
#if defined(OS_ANDROID) && defined(OFFICIAL_BUILD)
#define NOTIMPLEMENTED_POLICY 0
#else
// Select default policy: LOG(ERROR)
#define NOTIMPLEMENTED_POLICY 4
#endif
#endif

#if defined(COMPILER_GCC)
// On Linux, with GCC, we can use __PRETTY_FUNCTION__ to get the demangled name
// of the current function in the NOTIMPLEMENTED message.
#define NOTIMPLEMENTED_MSG "Not implemented reached in " << __PRETTY_FUNCTION__
#else
#define NOTIMPLEMENTED_MSG "NOT IMPLEMENTED"
#endif

#if NOTIMPLEMENTED_POLICY == 0
#define NOTIMPLEMENTED() BAIDU_EAT_STREAM_PARAMS
#elif NOTIMPLEMENTED_POLICY == 1
// TODO, figure out how to generate a warning
#define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 2
#define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 3
#define NOTIMPLEMENTED() NOTREACHED()
#elif NOTIMPLEMENTED_POLICY == 4
#define NOTIMPLEMENTED() LOG(ERROR) << NOTIMPLEMENTED_MSG
#elif NOTIMPLEMENTED_POLICY == 5
#define NOTIMPLEMENTED() do {                                   \
        static bool logged_once = false;                        \
        LOG_IF(ERROR, !logged_once) << NOTIMPLEMENTED_MSG;      \
        logged_once = true;                                     \
    } while(0);                                                 \
    BAIDU_EAT_STREAM_PARAMS
#endif

#if defined(NDEBUG) && defined(OS_CHROMEOS)
#define NOTREACHED() LOG(ERROR) << "NOTREACHED() hit in "       \
    << __FUNCTION__ << ". "
#else
#define NOTREACHED() DCHECK(false)
#endif

// Helper macro included by all *_EVERY_N macros.
#define BAIDU_LOG_IF_EVERY_N_IMPL(logifmacro, severity, condition, N)   \
    static ::butil::subtle::Atomic32 BAIDU_CONCAT(logeveryn_, __LINE__) = -1; \
    const static int BAIDU_CONCAT(logeveryn_sc_, __LINE__) = (N);       \
    const int BAIDU_CONCAT(logeveryn_c_, __LINE__) =                    \
        ::butil::subtle::NoBarrier_AtomicIncrement(&BAIDU_CONCAT(logeveryn_, __LINE__), 1); \
    logifmacro(severity, (condition) && BAIDU_CONCAT(logeveryn_c_, __LINE__) / \
               BAIDU_CONCAT(logeveryn_sc_, __LINE__) * BAIDU_CONCAT(logeveryn_sc_, __LINE__) \
               == BAIDU_CONCAT(logeveryn_c_, __LINE__))

// Helper macro included by all *_FIRST_N macros.
#define BAIDU_LOG_IF_FIRST_N_IMPL(logifmacro, severity, condition, N)   \
    static ::butil::subtle::Atomic32 BAIDU_CONCAT(logfstn_, __LINE__) = 0; \
    logifmacro(severity, (condition) && BAIDU_CONCAT(logfstn_, __LINE__) < N && \
               ::butil::subtle::NoBarrier_AtomicIncrement(&BAIDU_CONCAT(logfstn_, __LINE__), 1) <= N)

// Helper macro included by all *_EVERY_SECOND macros.
#define BAIDU_LOG_IF_EVERY_SECOND_IMPL(logifmacro, severity, condition) \
    static ::butil::subtle::Atomic64 BAIDU_CONCAT(logeverys_, __LINE__) = 0; \
    const int64_t BAIDU_CONCAT(logeverys_ts_, __LINE__) = ::butil::gettimeofday_us(); \
    const int64_t BAIDU_CONCAT(logeverys_seen_, __LINE__) = BAIDU_CONCAT(logeverys_, __LINE__); \
    logifmacro(severity, (condition) && BAIDU_CONCAT(logeverys_ts_, __LINE__) >= \
               (BAIDU_CONCAT(logeverys_seen_, __LINE__) + 1000000L) &&  \
               ::butil::subtle::NoBarrier_CompareAndSwap(                \
                   &BAIDU_CONCAT(logeverys_, __LINE__),                 \
                   BAIDU_CONCAT(logeverys_seen_, __LINE__),             \
                   BAIDU_CONCAT(logeverys_ts_, __LINE__))               \
               == BAIDU_CONCAT(logeverys_seen_, __LINE__))

// ===============================================================

// Print a log for at most once. (not present in glog)
// Almost zero overhead when the log was printed.
#ifndef LOG_ONCE
# define LOG_ONCE(severity) LOG_FIRST_N(severity, 1)
# define LOG_IF_ONCE(severity, condition) LOG_IF_FIRST_N(severity, condition, 1)
#endif

// Print a log after every N calls. First call always prints.
// Each call to this macro has a cost of relaxed atomic increment.
// The corresponding macro in glog is not thread-safe while this is.
#ifndef LOG_EVERY_N
# define LOG_EVERY_N(severity, N)                                \
     BAIDU_LOG_IF_EVERY_N_IMPL(LOG_IF, severity, true, N)
# define LOG_IF_EVERY_N(severity, condition, N)                  \
     BAIDU_LOG_IF_EVERY_N_IMPL(LOG_IF, severity, condition, N)
#endif

// Print logs for first N calls.
// Almost zero overhead when the log was printed for N times
// The corresponding macro in glog is not thread-safe while this is.
#ifndef LOG_FIRST_N
# define LOG_FIRST_N(severity, N)                                \
     BAIDU_LOG_IF_FIRST_N_IMPL(LOG_IF, severity, true, N)
# define LOG_IF_FIRST_N(severity, condition, N)                  \
     BAIDU_LOG_IF_FIRST_N_IMPL(LOG_IF, severity, condition, N)
#endif

// Print a log every second. (not present in glog). First call always prints.
// Each call to this macro has a cost of calling gettimeofday.
#ifndef LOG_EVERY_SECOND
# define LOG_EVERY_SECOND(severity)                                \
     BAIDU_LOG_IF_EVERY_SECOND_IMPL(LOG_IF, severity, true)
# define LOG_IF_EVERY_SECOND(severity, condition)                \
     BAIDU_LOG_IF_EVERY_SECOND_IMPL(LOG_IF, severity, condition)
#endif

#ifndef PLOG_EVERY_N
# define PLOG_EVERY_N(severity, N)                               \
     BAIDU_LOG_IF_EVERY_N_IMPL(PLOG_IF, severity, true, N)
# define PLOG_IF_EVERY_N(severity, condition, N)                 \
     BAIDU_LOG_IF_EVERY_N_IMPL(PLOG_IF, severity, condition, N)
#endif

#ifndef PLOG_FIRST_N
# define PLOG_FIRST_N(severity, N)                               \
     BAIDU_LOG_IF_FIRST_N_IMPL(PLOG_IF, severity, true, N)
# define PLOG_IF_FIRST_N(severity, condition, N)                 \
     BAIDU_LOG_IF_FIRST_N_IMPL(PLOG_IF, severity, condition, N)
#endif

#ifndef PLOG_ONCE
# define PLOG_ONCE(severity) PLOG_FIRST_N(severity, 1)
# define PLOG_IF_ONCE(severity, condition) PLOG_IF_FIRST_N(severity, condition, 1)
#endif

#ifndef PLOG_EVERY_SECOND
# define PLOG_EVERY_SECOND(severity)                             \
     BAIDU_LOG_IF_EVERY_SECOND_IMPL(PLOG_IF, severity, true)
# define PLOG_IF_EVERY_SECOND(severity, condition)                       \
     BAIDU_LOG_IF_EVERY_SECOND_IMPL(PLOG_IF, severity, condition)
#endif

// DEBUG_MODE is for uses like
//   if (DEBUG_MODE) foo.CheckThatFoo();
// instead of
//   #ifndef NDEBUG
//     foo.CheckThatFoo();
//   #endif
//
// We tie its state to ENABLE_DLOG.
enum { DEBUG_MODE = DCHECK_IS_ON() };


#endif  // MELON_COMMON_LOGGING_H_
