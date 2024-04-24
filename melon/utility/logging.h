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


// Date: 2012-10-08 23:53:50

// Merged chromium log and streaming log.

#ifndef MUTIL_LOGGING_H_
#define MUTIL_LOGGING_H_

#include "melon/utility/config.h"   // MELON_WITH_GLOG

#include <inttypes.h>
#include <string>
#include <cstring>
#include <sstream>
#include "melon/utility/macros.h"    // MELON_CONCAT
#include "melon/utility/atomicops.h" // Used by MLOG_EVERY_N, MLOG_FIRST_N etc
#include "melon/utility/time.h"      // gettimeofday_us()

#if MELON_WITH_GLOG
# include <glog/logging.h>
# include <glog/raw_logging.h>
// define macros that not implemented in glog
# ifndef DMCHECK_IS_ON   // glog didn't define DMCHECK_IS_ON in older version
#  if defined(NDEBUG)
#    define DMCHECK_IS_ON() 0
#  else
#    define DMCHECK_IS_ON() 1
#  endif  // NDEBUG
# endif // DMCHECK_IS_ON
# if DMCHECK_IS_ON()
#  define DPMLOG(...) PMLOG(__VA_ARGS__)
#  define DPMLOG_IF(...) PMLOG_IF(__VA_ARGS__)
#  define DPMCHECK(...) PMCHECK(__VA_ARGS__)
#  define DVPMLOG(...) VLOG(__VA_ARGS__)
# else 
#  define DPMLOG(...) DMLOG(__VA_ARGS__)
#  define DPMLOG_IF(...) DMLOG_IF(__VA_ARGS__)
#  define DPMCHECK(...) DMCHECK(__VA_ARGS__)
#  define DVPMLOG(...) DVMLOG(__VA_ARGS__)
# endif

#define LOG_AT(severity, file, line)                                    \
    google::LogMessage(file, line, google::severity).stream()

#else


#include <inttypes.h>
#include <gflags/gflags_declare.h>

#include "melon/utility/base_export.h"
#include "melon/utility/basictypes.h"
#include "melon/utility/debug/debugger.h"
#include "melon/utility/strings/string_piece.h"
#include "melon/utility/build_config.h"
#include "melon/utility/synchronization/lock.h"
//
// Optional message capabilities
// -----------------------------
// Assertion failed messages and fatal errors are displayed in a dialog box
// before the application exits. However, running this UI creates a message
// loop, which causes application messages to be processed and potentially
// dispatched to existing application windows. Since the application is in a
// bad state when this assertion dialog is displayed, these messages may not
// get processed and hang the dialog, or the application might go crazy.
//
// Therefore, it can be beneficial to display the error dialog in a separate
// process from the main application. When the logging system needs to display
// a fatal error dialog box, it will look for a program called
// "DebugMessage.exe" in the same directory as the application executable. It
// will run this application with the message as the command line, and will
// not include the name of the application as is traditional for easier
// parsing.
//
// The code for DebugMessage.exe is only one line. In WinMain, do:
//   MessageBox(NULL, GetCommandLineW(), L"Fatal Error", 0);
//
// If DebugMessage.exe is not found, the logging code will use a normal
// MessageBox, potentially causing the problems discussed above.


// Instructions
// ------------
//
// Make a bunch of macros for logging.  The way to log things is to stream
// things to MLOG(<a particular severity level>).  E.g.,
//
//   MLOG(INFO) << "Found " << num_cookies << " cookies";
//
// You can also do conditional logging:
//
//   LOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
//
// The MCHECK(condition) macro is active in both debug and release builds and
// effectively performs a MLOG(FATAL) which terminates the process and
// generates a crashdump unless a debugger is attached.
//
// There are also "debug mode" logging macros like the ones above:
//
//   DMLOG(INFO) << "Found cookies";
//
//   DMLOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
//
// All "debug mode" logging is compiled away to nothing for non-debug mode
// compiles.  LOG_IF and development flags also work well together
// because the code can be compiled away sometimes.
//
// We also have
//
//   DLOG_ASSERT(assertion);
//   DMLOG_ASSERT(assertion);
//
// which is syntactic sugar for {,D}LOG_IF(FATAL, assert fails) << assertion;
//
// There are "verbose level" logging macros.  They look like
//
//   VLOG(1) << "I'm printed when you run the program with --v=1 or more";
//   VLOG(2) << "I'm printed when you run the program with --v=2 or more";
//
// These always log at the INFO log level (when they log at all).
// The verbose logging can also be turned on module-by-module.  For instance,
//    --vmodule=profile=2,icon_loader=1,browser_*=3,*/chromeos/*=4 --v=0
// will cause:
//   a. VLOG(2) and lower messages to be printed from profile.{h,cc}
//   b. VLOG(1) and lower messages to be printed from icon_loader.{h,cc}
//   c. VLOG(3) and lower messages to be printed from files prefixed with
//      "browser"
//   d. VLOG(4) and lower messages to be printed from files under a
//     "chromeos" directory.
//   e. VLOG(0) and lower messages to be printed from elsewhere
//
// The wildcarding functionality shown by (c) supports both '*' (match
// 0 or more characters) and '?' (match any single character)
// wildcards.  Any pattern containing a forward or backward slash will
// be tested against the whole pathname and not just the module.
// E.g., "*/foo/bar/*=2" would change the logging level for all code
// in source files under a "foo/bar" directory.
//
// There's also VLOG_IS_ON(n) "verbose level" condition macro. To be used as
//
//   if (VLOG_IS_ON(2)) {
//     // do some logging preparation and logging
//     // that can't be accomplished with just VLOG(2) << ...;
//   }
//
// There is also a VLOG_IF "verbose level" condition macro for sample
// cases, when some extra computation and preparation for logs is not
// needed.
//
//   VLOG_IF(1, (size > 1024))
//      << "I'm printed when size is more than 1024 and when you run the "
//         "program with --v=1 or more";
//
// Lastly, there is:
//
//   PMLOG(ERROR) << "Couldn't do foo";
//   DPMLOG(ERROR) << "Couldn't do foo";
//   PMLOG_IF(ERROR, cond) << "Couldn't do foo";
//   DPMLOG_IF(ERROR, cond) << "Couldn't do foo";
//   PMCHECK(condition) << "Couldn't do foo";
//   DPMCHECK(condition) << "Couldn't do foo";
//
// which append the last system error to the message in string form (taken from
// GetLastError() on Windows and errno on POSIX).
//
// The supported severity levels for macros that allow you to specify one
// are (in increasing order of severity) INFO, WARNING, ERROR, and FATAL.
//
// Very important: logging a message at the FATAL severity level causes
// the program to terminate (after the message is logged).
//
// There is the special severity of DFATAL, which logs FATAL in debug mode,
// ERROR in normal mode.

namespace logging {

// TODO(avi): do we want to do a unification of character types here?
#if defined(OS_WIN)
typedef wchar_t LogChar;
#else
typedef char LogChar;
#endif

// Where to record logging output? A flat file and/or system debug log
// via OutputDebugString.
enum LoggingDestination {
    LOG_TO_NONE             = 0,
    LOG_TO_FILE             = 1 << 0,
    LOG_TO_SYSTEM_DEBUG_LOG = 1 << 1,

    LOG_TO_ALL = LOG_TO_FILE | LOG_TO_SYSTEM_DEBUG_LOG,

    // On Windows, use a file next to the exe; on POSIX platforms, where
    // it may not even be possible to locate the executable on disk, use
    // stderr.
#if defined(OS_WIN)
    LOG_DEFAULT = LOG_TO_FILE,
#elif defined(OS_POSIX)
    LOG_DEFAULT = LOG_TO_SYSTEM_DEBUG_LOG,
#endif
};

// Indicates that the log file should be locked when being written to.
// Unless there is only one single-threaded process that is logging to
// the log file, the file should be locked during writes to make each
// log output atomic. Other writers will block.
//
// All processes writing to the log file must have their locking set for it to
// work properly. Defaults to LOCK_LOG_FILE.
enum LogLockingState { LOCK_LOG_FILE, DONT_LOCK_LOG_FILE };

// On startup, should we delete or append to an existing log file (if any)?
// Defaults to APPEND_TO_OLD_LOG_FILE.
enum OldFileDeletionState { DELETE_OLD_LOG_FILE, APPEND_TO_OLD_LOG_FILE };

struct MUTIL_EXPORT LoggingSettings {
    // The defaults values are:
    //
    //  logging_dest: LOG_DEFAULT
    //  log_file:     NULL
    //  lock_log:     LOCK_LOG_FILE
    //  delete_old:   APPEND_TO_OLD_LOG_FILE
    LoggingSettings();

    LoggingDestination logging_dest;

    // The three settings below have an effect only when LOG_TO_FILE is
    // set in |logging_dest|.
    const LogChar* log_file;
    LogLockingState lock_log;
    OldFileDeletionState delete_old;
};

// Implementation of the InitLogging() method declared below. 
MUTIL_EXPORT bool BaseInitLoggingImpl(const LoggingSettings& settings);

// Sets the log file name and other global logging state. Calling this function
// is recommended, and is normally done at the beginning of application init.
// If you don't call it, all the flags will be initialized to their default
// values, and there is a race condition that may leak a critical section
// object if two threads try to do the first log at the same time.
// See the definition of the enums above for descriptions and default values.
//
// The default log file is initialized to "<process-name>.log" on linux and
// "debug.log" otherwise.
//
// This function may be called a second time to re-direct logging (e.g after
// loging in to a user partition), however it should never be called more than
// twice.
inline bool InitLogging(const LoggingSettings& settings) {
    return BaseInitLoggingImpl(settings);
}

// Sets the log level. Anything at or above this level will be written to the
// log file/displayed to the user (if applicable). Anything below this level
// will be silently ignored. The log level defaults to 0 (everything is logged
// up to level INFO) if this function is not called.
MUTIL_EXPORT void SetMinLogLevel(int level);

// Gets the current log level.
MUTIL_EXPORT int GetMinLogLevel();

// Sets whether or not you'd like to see fatal debug messages popped up in
// a dialog box or not.
// Dialogs are not shown by default.
MUTIL_EXPORT void SetShowErrorDialogs(bool enable_dialogs);

// Sets the Log Assert Handler that will be used to notify of check failures.
// The default handler shows a dialog box and then terminate the process,
// however clients can use this function to override with their own handling
// (e.g. a silent one for Unit Tests)
typedef void (*LogAssertHandler)(const std::string& str);
MUTIL_EXPORT void SetLogAssertHandler(LogAssertHandler handler);

class LogSink {
public:
    LogSink() {}
    virtual ~LogSink() {}
    // Called when a log is ready to be written out.
    // Returns true to stop further processing.
    virtual bool OnLogMessage(int severity, const char* file, int line,
                              const mutil::StringPiece& log_content) = 0;
    virtual bool OnLogMessage(int severity, const char* file,
                              int line, const char* func,
                              const mutil::StringPiece& log_content) {
        return true;
    }
private:
    DISALLOW_COPY_AND_ASSIGN(LogSink);
};

// Sets the LogSink that gets passed every log message before
// it's sent to default log destinations.
// This function is thread-safe and waits until current LogSink is not used
// anymore.
// Returns previous sink.
MUTIL_EXPORT LogSink* SetLogSink(LogSink* sink);

// Print |content| with other info into |os|.
void PrintLog(std::ostream& os,
              int severity, const char* file, int line,
              const mutil::StringPiece& content);

void PrintLog(std::ostream& os,
              int severity, const char* file, int line,
              const char* func, const mutil::StringPiece& content);

// The LogSink mainly for unit-testing. Logs will be appended to it.
class StringSink : public LogSink, public std::string {
public:
    bool OnLogMessage(int severity, const char* file, int line,
                      const mutil::StringPiece& log_content) override;

    bool OnLogMessage(int severity, const char* file,
                      int line, const char* func,
                      const mutil::StringPiece& log_content) override;
private:
    mutil::Lock _lock;
};

typedef int LogSeverity;
const LogSeverity BLOG_VERBOSE = -1;  // This is level 1 verbosity
// Note: the log severities are used to index into the array of names,
// see log_severity_names.
const LogSeverity BLOG_INFO = 0;
const LogSeverity BLOG_NOTICE = 1;
const LogSeverity BLOG_WARNING = 2;
const LogSeverity BLOG_ERROR = 3;
const LogSeverity BLOG_FATAL = 4;
const int LOG_NUM_SEVERITIES = 5;

// COMBLOG_TRACE is just INFO
const LogSeverity BLOG_TRACE = BLOG_INFO;

// COMBLOG_DEBUG equals INFO in debug mode and verbose in normal mode.
#ifndef NDEBUG
const LogSeverity BLOG_DEBUG = BLOG_INFO;
#else
const LogSeverity BLOG_DEBUG = BLOG_VERBOSE;
#endif

// BLOG_DFATAL is BLOG_FATAL in debug mode, ERROR in normal mode
#ifndef NDEBUG
const LogSeverity BLOG_DFATAL = BLOG_FATAL;
#else
const LogSeverity BLOG_DFATAL = BLOG_ERROR;
#endif

// A few definitions of macros that don't generate much code. These are used
// by MLOG() and LOG_IF, etc. Since these are used all over our code, it's
// better to have compact code for these operations.
#define MELON_COMPACT_LOG_EX(severity, ClassName, ...)  \
    ::logging::ClassName(__FILE__, __LINE__,  __func__, \
    ::logging::BLOG_##severity, ##__VA_ARGS__)

#define MELON_COMPACK_LOG(severity)             \
    MELON_COMPACT_LOG_EX(severity, LogMessage)

#if defined(OS_WIN)
// wingdi.h defines ERROR to be 0. When we call MLOG(ERROR), it gets
// substituted with 0, and it expands to MELON_COMPACK_LOG(0). To allow us
// to keep using this syntax, we define this macro to do the same thing
// as MELON_COMPACK_LOG(ERROR), and also define ERROR the same way that
// the Windows SDK does for consistency.
#undef ERROR
#define ERROR 0
// Needed for LOG_IS_ON(ERROR).
const LogSeverity BLOG_0 = BLOG_ERROR;
#endif

// As special cases, we can assume that LOG_IS_ON(FATAL) always holds. Also,
// LOG_IS_ON(DFATAL) always holds in debug mode. In particular, MCHECK()s will
// always fire if they fail.
#define LOG_IS_ON(severity)                                     \
    (::logging::BLOG_##severity >= ::logging::GetMinLogLevel())

#if defined(__GNUC__)
// We emit an anonymous static int* variable at every VLOG_IS_ON(n) site.
// (Normally) the first time every VLOG_IS_ON(n) site is hit,
// we determine what variable will dynamically control logging at this site:
// it's either FLAGS_verbose or an appropriate internal variable
// matching the current source file that represents results of
// parsing of --vmodule flag and/or SetVLOGLevel calls.
# define MELON_VLOG_IS_ON(verbose_level, filepath)                      \
    ({ static const int* vlocal = &::logging::VLOG_UNINITIALIZED;       \
        const int saved_verbose_level = (verbose_level);                \
        (saved_verbose_level >= 0)/*VLOG(-1) is forbidden*/ &&          \
            (*vlocal >= saved_verbose_level) &&                         \
            ((vlocal != &::logging::VLOG_UNINITIALIZED) ||              \
             (::logging::add_vlog_site(&vlocal, filepath, __LINE__,     \
                                       saved_verbose_level))); })
#else
// GNU extensions not available, so we do not support --vmodule.
// Dynamic value of FLAGS_verbose always controls the logging level.
# define MELON_VLOG_IS_ON(verbose_level, filepath)      \
    (::logging::FLAGS_v >= (verbose_level))
#endif

#define VLOG_IS_ON(verbose_level) MELON_VLOG_IS_ON(verbose_level, __FILE__)

DECLARE_int32(v);

extern const int VLOG_UNINITIALIZED;

// Called to initialize a VLOG callsite.
bool add_vlog_site(const int** v, const LogChar* filename,
                   int line_no, int required_v);

class VLogSitePrinter {
public:
    struct Site {
        int current_verbose_level;
        int required_verbose_level;
        int line_no;
        std::string full_module;
    };

    virtual void print(const Site& site) = 0;
    virtual ~VLogSitePrinter() = default;
};

void print_vlog_sites(VLogSitePrinter*);

// Helper macro which avoids evaluating the arguments to a stream if
// the condition doesn't hold.
#define MELON_LAZY_STREAM(stream, condition)                            \
    !(condition) ? (void) 0 : ::logging::LogMessageVoidify() & (stream)

// We use the preprocessor's merging operator, "##", so that, e.g.,
// MLOG(INFO) becomes the token MELON_COMPACK_LOG(INFO).  There's some funny
// subtle difference between ostream member streaming functions (e.g.,
// ostream::operator<<(int) and ostream non-member streaming functions
// (e.g., ::operator<<(ostream&, string&): it turns out that it's
// impossible to stream something like a string directly to an unnamed
// ostream. We employ a neat hack by calling the stream() member
// function of LogMessage which seems to avoid the problem.
#define LOG_STREAM(severity) MELON_COMPACK_LOG(severity).stream()

#define MLOG(severity)                                                   \
    MELON_LAZY_STREAM(LOG_STREAM(severity), LOG_IS_ON(severity))
#define LOG_IF(severity, condition)                                     \
    MELON_LAZY_STREAM(LOG_STREAM(severity), LOG_IS_ON(severity) && (condition))

// FIXME(gejun): Should always crash.
#define DLOG_ASSERT(condition)                                           \
    LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "

#define SYSLOG(severity) MLOG(severity)
#define SYSLOG_IF(severity, condition) LOG_IF(severity, condition)
#define SYSLOG_EVERY_N(severity, N) MLOG_EVERY_N(severity, N)
#define SYSLOG_IF_EVERY_N(severity, condition, N) LOG_IF_EVERY_N(severity, condition, N)
#define SYSLOG_FIRST_N(severity, N) MLOG_FIRST_N(severity, N)
#define SYSLOG_IF_FIRST_N(severity, condition, N) LOG_IF_FIRST_N(severity, condition, N)
#define SYSLOG_ONCE(severity) MLOG_FIRST_N(severity, 1)
#define SYSLOG_IF_ONCE(severity, condition) LOG_IF_FIRST_N(severity, condition, 1)
#define SYSLOG_EVERY_SECOND(severity) MLOG_EVERY_SECOND(severity)
#define SYSLOG_IF_EVERY_SECOND(severity, condition) LOG_IF_EVERY_SECOND(severity, condition)

#define SYSLOG_ASSERT(condition)                                        \
    SYSLOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "

// file/line can be specified at running-time. This is useful for printing
// logs with known file/line inside a LogSink or LogMessageHandler
#define LOG_AT_SELECTOR(_1, _2, _3, _4, NAME, ...) NAME

#define LOG_AT_STREAM1(severity, file, line)                                 \
    ::logging::LogMessage(file, line, ::logging::BLOG_##severity).stream()
#define LOG_AT_STREAM2(severity, file, line, func)                           \
    ::logging::LogMessage(file, line, func, ::logging::BLOG_##severity).stream()
#define LOG_AT_STREAM(...) LOG_AT_SELECTOR(__VA_ARGS__, LOG_AT_STREAM2, LOG_AT_STREAM1)(__VA_ARGS__)

#define LOG_AT1(severity, file, line)                                        \
    MELON_LAZY_STREAM(LOG_AT_STREAM(severity, file, line), LOG_IS_ON(severity))
#define LOG_AT2(severity, file, line, func)                                   \
    MELON_LAZY_STREAM(LOG_AT_STREAM(severity, file, line, func), LOG_IS_ON(severity))
#define LOG_AT(...) LOG_AT_SELECTOR(__VA_ARGS__, LOG_AT2, LOG_AT1)(__VA_ARGS__)


// The VLOG macros log with negative verbosities.
#define VLOG_STREAM(verbose_level)                                      \
    ::logging::LogMessage(__FILE__, __LINE__, __func__, -(verbose_level)).stream()

#define VLOG(verbose_level)                                             \
    MELON_LAZY_STREAM(VLOG_STREAM(verbose_level), VLOG_IS_ON(verbose_level))
#define VLOG_IF(verbose_level, condition)                       \
    MELON_LAZY_STREAM(VLOG_STREAM(verbose_level),               \
                      VLOG_IS_ON(verbose_level) && (condition))

#define VLOG_EVERY_N(verbose_level, N)                                  \
    MELON_LOG_IF_EVERY_N_IMPL(VLOG_IF, verbose_level, true, N)
#define VLOG_IF_EVERY_N(verbose_level, condition, N)                    \
    MELON_LOG_IF_EVERY_N_IMPL(VLOG_IF, verbose_level, condition, N)

#define VLOG_FIRST_N(verbose_level, N)                                  \
    MELON_LOG_IF_FIRST_N_IMPL(VLOG_IF, verbose_level, true, N)
#define VLOG_IF_FIRST_N(verbose_level, condition, N)                    \
    MELON_LOG_IF_FIRST_N_IMPL(VLOG_IF, verbose_level, condition, N)

#define VLOG_ONCE(verbose_level) VLOG_FIRST_N(verbose_level, 1)
#define VLOG_IF_ONCE(verbose_level, condition) VLOG_IF_FIRST_N(verbose_level, condition, 1)

#define VLOG_EVERY_SECOND(verbose_level)                        \
    MELON_LOG_IF_EVERY_SECOND_IMPL(VLOG_IF, verbose_level, true)
#define VLOG_IF_EVERY_SECOND(verbose_level, condition)                  \
    MELON_LOG_IF_EVERY_SECOND_IMPL(VLOG_IF, verbose_level, condition)

#if defined (OS_WIN)
#define VPLOG_STREAM(verbose_level)                                     \
     ::logging::Win32ErrorLogMessage(__FILE__, __LINE__, __func__, -verbose_level, \
                                     ::logging::GetLastSystemErrorCode()).stream()
#elif defined(OS_POSIX)
#define VPLOG_STREAM(verbose_level)                                     \
    ::logging::ErrnoLogMessage(__FILE__, __LINE__, __func__, -verbose_level,      \
                               ::logging::GetLastSystemErrorCode()).stream()
#endif

#define VPLOG(verbose_level)                                            \
    MELON_LAZY_STREAM(VPLOG_STREAM(verbose_level), VLOG_IS_ON(verbose_level))

#define VPLOG_IF(verbose_level, condition)                      \
    MELON_LAZY_STREAM(VPLOG_STREAM(verbose_level),              \
                      VLOG_IS_ON(verbose_level) && (condition))

#if defined(OS_WIN)
#define PLOG_STREAM(severity)                                           \
    MELON_COMPACT_LOG_EX(severity, Win32ErrorLogMessage,                \
                         ::logging::GetLastSystemErrorCode()).stream()
#elif defined(OS_POSIX)
#define PLOG_STREAM(severity)                                           \
    MELON_COMPACT_LOG_EX(severity, ErrnoLogMessage,                     \
                         ::logging::GetLastSystemErrorCode()).stream()
#endif

#define PMLOG(severity)                                                  \
    MELON_LAZY_STREAM(PLOG_STREAM(severity), LOG_IS_ON(severity))
#define PMLOG_IF(severity, condition)                                    \
    MELON_LAZY_STREAM(PLOG_STREAM(severity), LOG_IS_ON(severity) && (condition))

// The actual stream used isn't important.
#define MELON_EAT_STREAM_PARAMS                                           \
    true ? (void) 0 : ::logging::LogMessageVoidify() & LOG_STREAM(FATAL)

// MCHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by NDEBUG, so the check will be executed regardless of
// compilation mode.
//
// We make sure MCHECK et al. always evaluates their arguments, as
// doing MCHECK(FunctionWithSideEffect()) is a common idiom.

#if defined(OFFICIAL_BUILD) && defined(NDEBUG)

// Make all MCHECK functions discard their log strings to reduce code
// bloat for official release builds.

// TODO(akalin): This would be more valuable if there were some way to
// remove BreakDebugger() from the backtrace, perhaps by turning it
// into a macro (like __debugbreak() on Windows).
#define MCHECK(condition)                                                \
    !(condition) ? ::mutil::debug::BreakDebugger() : MELON_EAT_STREAM_PARAMS

#define PMCHECK(condition) MCHECK(condition)

#define MELON_CHECK_OP(name, op, val1, val2) MCHECK((val1) op (val2))

#else

#define MCHECK(condition)                                        \
    MELON_LAZY_STREAM(LOG_STREAM(FATAL).SetCheck(), !(condition))     \
    << "Check failed: " #condition ". "

#define PMCHECK(condition)                                       \
    MELON_LAZY_STREAM(PLOG_STREAM(FATAL).SetCheck(), !(condition))    \
    << "Check failed: " #condition ". "

// Helper macro for binary operators.
// Don't use this macro directly in your code, use MCHECK_EQ et al below.
//
// TODO(akalin): Rewrite this so that constructs like if (...)
// MCHECK_EQ(...) else { ... } work properly.
#define MELON_CHECK_OP(name, op, val1, val2)                                  \
    if (std::string* _result =                                          \
        ::logging::Check##name##Impl((val1), (val2),                    \
                                     #val1 " " #op " " #val2))          \
        ::logging::LogMessage(__FILE__, __LINE__, __func__, _result).stream().SetCheck()

#endif

// Build the error message string.  This is separate from the "Impl"
// function template because it is not performance critical and so can
// be out of line, while the "Impl" code should be inline.  Caller
// takes ownership of the returned string.
template<class t1, class t2>
std::string* MakeCheckOpString(const t1& v1, const t2& v2, const char* names) {
    std::ostringstream ss;
    ss << names << " (" << v1 << " vs " << v2 << "). ";
    std::string* msg = new std::string(ss.str());
    return msg;
}

// MSVC doesn't like complex extern templates and DLLs.
#if !defined(COMPILER_MSVC)
// Commonly used instantiations of MakeCheckOpString<>. Explicitly instantiated
// in logging.cc.
extern template MUTIL_EXPORT std::string* MakeCheckOpString<int, int>(
    const int&, const int&, const char* names);
extern template MUTIL_EXPORT
std::string* MakeCheckOpString<unsigned long, unsigned long>(
    const unsigned long&, const unsigned long&, const char* names);
extern template MUTIL_EXPORT
std::string* MakeCheckOpString<unsigned long, unsigned int>(
    const unsigned long&, const unsigned int&, const char* names);
extern template MUTIL_EXPORT
std::string* MakeCheckOpString<unsigned int, unsigned long>(
    const unsigned int&, const unsigned long&, const char* names);
extern template MUTIL_EXPORT
std::string* MakeCheckOpString<std::string, std::string>(
    const std::string&, const std::string&, const char* name);
#endif

// Helper functions for MELON_CHECK_OP macro.
// The (int, int) specialization works around the issue that the compiler
// will not instantiate the template version of the function on values of
// unnamed enum type - see comment below.
#define MELON_DEFINE_CHECK_OP_IMPL(name, op)                            \
    template <class t1, class t2>                                       \
    inline std::string* Check##name##Impl(const t1& v1, const t2& v2,   \
                                          const char* names) {          \
        if (v1 op v2) return NULL;                                      \
        else return MakeCheckOpString(v1, v2, names);                   \
    }                                                                   \
    inline std::string* Check##name##Impl(int v1, int v2, const char* names) { \
        if (v1 op v2) return NULL;                                      \
        else return MakeCheckOpString(v1, v2, names);                   \
    }
MELON_DEFINE_CHECK_OP_IMPL(EQ, ==)
MELON_DEFINE_CHECK_OP_IMPL(NE, !=)
MELON_DEFINE_CHECK_OP_IMPL(LE, <=)
MELON_DEFINE_CHECK_OP_IMPL(LT, < )
MELON_DEFINE_CHECK_OP_IMPL(GE, >=)
MELON_DEFINE_CHECK_OP_IMPL(GT, > )
#undef MELON_DEFINE_CHECK_OP_IMPL

#define MCHECK_EQ(val1, val2) MELON_CHECK_OP(EQ, ==, val1, val2)
#define MCHECK_NE(val1, val2) MELON_CHECK_OP(NE, !=, val1, val2)
#define MCHECK_LE(val1, val2) MELON_CHECK_OP(LE, <=, val1, val2)
#define MCHECK_LT(val1, val2) MELON_CHECK_OP(LT, < , val1, val2)
#define MCHECK_GE(val1, val2) MELON_CHECK_OP(GE, >=, val1, val2)
#define MCHECK_GT(val1, val2) MELON_CHECK_OP(GT, > , val1, val2)

#if defined(NDEBUG) && !defined(DMCHECK_ALWAYS_ON)
#define DMCHECK_IS_ON() 0
#else
#define DMCHECK_IS_ON() 1
#endif

#define ENABLE_DMLOG DMCHECK_IS_ON()

// Definitions for DMLOG et al.

// Need to be this way because `condition' may contain variables that is only
// defined in debug mode.
#if ENABLE_DMLOG
#define DMLOG_IS_ON(severity) LOG_IS_ON(severity)
#define DMLOG_IF(severity, condition)                    \
    LOG_IF(severity, ENABLE_DMLOG && (condition))
#define DMLOG_ASSERT(condition) DLOG_ASSERT(!ENABLE_DMLOG || condition)
#define DPMLOG_IF(severity, condition)                   \
    PMLOG_IF(severity, ENABLE_DMLOG && (condition))
#define DVMLOG_IF(verbose_level, condition)               \
    VLOG_IF(verbose_level, ENABLE_DMLOG && (condition))
#define DVPMLOG_IF(verbose_level, condition)      \
    VPLOG_IF(verbose_level, ENABLE_DMLOG && (condition))
#else  // ENABLE_DMLOG
#define DMLOG_IS_ON(severity) false
#define DMLOG_IF(severity, condition) MELON_EAT_STREAM_PARAMS
#define DMLOG_ASSERT(condition) MELON_EAT_STREAM_PARAMS
#define DPMLOG_IF(severity, condition) MELON_EAT_STREAM_PARAMS
#define DVMLOG_IF(verbose_level, condition) MELON_EAT_STREAM_PARAMS
#define DVPMLOG_IF(verbose_level, condition) MELON_EAT_STREAM_PARAMS
#endif  // ENABLE_DMLOG

#define DMLOG(severity)                                          \
    MELON_LAZY_STREAM(LOG_STREAM(severity), DMLOG_IS_ON(severity))
#define DMLOG_EVERY_N(severity, N)                               \
    MELON_LOG_IF_EVERY_N_IMPL(DMLOG_IF, severity, true, N)
#define DMLOG_IF_EVERY_N(severity, condition, N)                 \
    MELON_LOG_IF_EVERY_N_IMPL(DMLOG_IF, severity, condition, N)
#define DMLOG_FIRST_N(severity, N)                               \
    MELON_LOG_IF_FIRST_N_IMPL(DMLOG_IF, severity, true, N)
#define DMLOG_IF_FIRST_N(severity, condition, N)                 \
    MELON_LOG_IF_FIRST_N_IMPL(DMLOG_IF, severity, condition, N)
#define DMLOG_ONCE(severity) DMLOG_FIRST_N(severity, 1)
#define DMLOG_IF_ONCE(severity, condition) DMLOG_IF_FIRST_N(severity, condition, 1)
#define DMLOG_EVERY_SECOND(severity)                             \
    MELON_LOG_IF_EVERY_SECOND_IMPL(DMLOG_IF, severity, true)
#define DMLOG_IF_EVERY_SECOND(severity, condition)                       \
    MELON_LOG_IF_EVERY_SECOND_IMPL(DMLOG_IF, severity, condition)

#define DPMLOG(severity)                                         \
    MELON_LAZY_STREAM(PLOG_STREAM(severity), DMLOG_IS_ON(severity))
#define DPMLOG_EVERY_N(severity, N)                               \
    MELON_LOG_IF_EVERY_N_IMPL(DPMLOG_IF, severity, true, N)
#define DPMLOG_IF_EVERY_N(severity, condition, N)                 \
    MELON_LOG_IF_EVERY_N_IMPL(DPMLOG_IF, severity, condition, N)
#define DPMLOG_FIRST_N(severity, N)                               \
    MELON_LOG_IF_FIRST_N_IMPL(DPMLOG_IF, severity, true, N)
#define DPLOG_IF_FIRST_N(severity, condition, N)                 \
    MELON_LOG_IF_FIRST_N_IMPL(DPMLOG_IF, severity, condition, N)
#define DPMLOG_ONCE(severity) DPMLOG_FIRST_N(severity, 1)
#define DPMLOG_IF_ONCE(severity, condition) DPLOG_IF_FIRST_N(severity, condition, 1)
#define DPMLOG_EVERY_SECOND(severity)                             \
    MELON_LOG_IF_EVERY_SECOND_IMPL(DPMLOG_IF, severity, true)
#define DPMLOG_IF_EVERY_SECOND(severity, condition)                       \
    MELON_LOG_IF_EVERY_SECOND_IMPL(DPMLOG_IF, severity, condition)

#define DVMLOG(verbose_level) DVMLOG_IF(verbose_level, VLOG_IS_ON(verbose_level))
#define DVMLOG_EVERY_N(verbose_level, N)                               \
    MELON_LOG_IF_EVERY_N_IMPL(DVMLOG_IF, verbose_level, true, N)
#define DVMLOG_IF_EVERY_N(verbose_level, condition, N)                 \
    MELON_LOG_IF_EVERY_N_IMPL(DVMLOG_IF, verbose_level, condition, N)
#define DVMLOG_FIRST_N(verbose_level, N)                               \
    MELON_LOG_IF_FIRST_N_IMPL(DVMLOG_IF, verbose_level, true, N)
#define DVMLOG_IF_FIRST_N(verbose_level, condition, N)                 \
    MELON_LOG_IF_FIRST_N_IMPL(DVMLOG_IF, verbose_level, condition, N)
#define DVMLOG_ONCE(verbose_level) DVMLOG_FIRST_N(verbose_level, 1)
#define DVMLOG_IF_ONCE(verbose_level, condition) DVMLOG_IF_FIRST_N(verbose_level, condition, 1)
#define DVMLOG_EVERY_SECOND(verbose_level)                             \
    MELON_LOG_IF_EVERY_SECOND_IMPL(DVMLOG_IF, verbose_level, true)
#define DVMLOG_IF_EVERY_SECOND(verbose_level, condition)                       \
    MELON_LOG_IF_EVERY_SECOND_IMPL(DVMLOG_IF, verbose_level, condition)

#define DVPMLOG(verbose_level) DVPMLOG_IF(verbose_level, VLOG_IS_ON(verbose_level))
#define DVPMLOG_EVERY_N(verbose_level, N)                               \
    MELON_LOG_IF_EVERY_N_IMPL(DVPMLOG_IF, verbose_level, true, N)
#define DVPMLOG_IF_EVERY_N(verbose_level, condition, N)                 \
    MELON_LOG_IF_EVERY_N_IMPL(DVPMLOG_IF, verbose_level, condition, N)
#define DVPMLOG_FIRST_N(verbose_level, N)                               \
    MELON_LOG_IF_FIRST_N_IMPL(DVPMLOG_IF, verbose_level, true, N)
#define DVPMLOG_IF_FIRST_N(verbose_level, condition, N)                 \
    MELON_LOG_IF_FIRST_N_IMPL(DVPMLOG_IF, verbose_level, condition, N)
#define DVPMLOG_ONCE(verbose_level) DVPMLOG_FIRST_N(verbose_level, 1)
#define DVPMLOG_IF_ONCE(verbose_level, condition) DVPMLOG_IF_FIRST_N(verbose_level, condition, 1)
#define DVPMLOG_EVERY_SECOND(verbose_level)                             \
    MELON_LOG_IF_EVERY_SECOND_IMPL(DVPMLOG_IF, verbose_level, true)
#define DVPMLOG_IF_EVERY_SECOND(verbose_level, condition)                       \
    MELON_LOG_IF_EVERY_SECOND_IMPL(DVPMLOG_IF, verbose_level, condition)

// You can assign virtual path to VLOG instead of physical filename.
// [public/foo/bar.cpp]
// VMLOG2("a/b/c", 2) << "being filtered by a/b/c rather than public/foo/bar";
#define VMLOG2(virtual_path, verbose_level)                              \
    MELON_LAZY_STREAM(VLOG_STREAM(verbose_level),                       \
                      MELON_VLOG_IS_ON(verbose_level, virtual_path))

#define VMLOG2_IF(virtual_path, verbose_level, condition)                \
    MELON_LAZY_STREAM(VLOG_STREAM(verbose_level),                       \
                      MELON_VLOG_IS_ON(verbose_level, virtual_path) && (condition))

#define DVMLOG2(virtual_path, verbose_level)             \
    VMLOG2_IF(virtual_path, verbose_level, ENABLE_DMLOG)

#define DVMLOG2_IF(virtual_path, verbose_level, condition)               \
    VMLOG2_IF(virtual_path, verbose_level, ENABLE_DMLOG && (condition))

#define VPMLOG2(virtual_path, verbose_level)                             \
    MELON_LAZY_STREAM(VPLOG_STREAM(verbose_level),                      \
                      MELON_VLOG_IS_ON(verbose_level, virtual_path))

#define VPMLOG2_IF(virtual_path, verbose_level, condition)               \
    MELON_LAZY_STREAM(VPLOG_STREAM(verbose_level),                      \
                      MELON_VLOG_IS_ON(verbose_level, virtual_path) && (condition))

#define DVPMLOG2(virtual_path, verbose_level)                            \
    VPMLOG2_IF(virtual_path, verbose_level, ENABLE_DMLOG)

#define DVPMLOG2_IF(virtual_path, verbose_level, condition)              \
    VPMLOG2_IF(virtual_path, verbose_level, ENABLE_DMLOG && (condition))

// Definitions for DMCHECK et al.

#if DMCHECK_IS_ON()

const LogSeverity BLOG_DMCHECK = BLOG_FATAL;

#else  // DMCHECK_IS_ON

const LogSeverity BLOG_DMCHECK = BLOG_INFO;

#endif  // DMCHECK_IS_ON

// DMCHECK et al. make sure to reference |condition| regardless of
// whether DCHECKs are enabled; this is so that we don't get unused
// variable warnings if the only use of a variable is in a DMCHECK.
// This behavior is different from DMLOG_IF et al.

#define DMCHECK(condition)                                               \
    MELON_LAZY_STREAM(LOG_STREAM(DMCHECK), DMCHECK_IS_ON() && !(condition)) \
    << "Check failed: " #condition ". "

#define DPMCHECK(condition)                                              \
    MELON_LAZY_STREAM(PLOG_STREAM(DMCHECK), DMCHECK_IS_ON() && !(condition)) \
    << "Check failed: " #condition ". "

// Helper macro for binary operators.
// Don't use this macro directly in your code, use DMCHECK_EQ et al below.
#define MELON_DCHECK_OP(name, op, val1, val2)                           \
    if (DMCHECK_IS_ON())                                                   \
        if (std::string* _result =                                      \
            ::logging::Check##name##Impl((val1), (val2),                \
                                         #val1 " " #op " " #val2))      \
            ::logging::LogMessage(                                      \
                __FILE__, __LINE__, __func__,                           \
                ::logging::BLOG_DMCHECK,                                 \
                _result).stream()

// Equality/Inequality checks - compare two values, and log a
// BLOG_DMCHECK message including the two values when the result is not
// as expected.  The values must have operator<<(ostream, ...)
// defined.
//
// You may append to the error message like so:
//   DMCHECK_NE(1, 2) << ": The world must be ending!";
//
// We are very careful to ensure that each argument is evaluated exactly
// once, and that anything which is legal to pass as a function argument is
// legal here.  In particular, the arguments may be temporary expressions
// which will end up being destroyed at the end of the apparent statement,
// for example:
//   DMCHECK_EQ(string("abc")[1], 'b');
//
// WARNING: These may not compile correctly if one of the arguments is a pointer
// and the other is NULL. To work around this, simply static_cast NULL to the
// type of the desired pointer.

#define DMCHECK_EQ(val1, val2) MELON_DCHECK_OP(EQ, ==, val1, val2)
#define DMCHECK_NE(val1, val2) MELON_DCHECK_OP(NE, !=, val1, val2)
#define DMCHECK_LE(val1, val2) MELON_DCHECK_OP(LE, <=, val1, val2)
#define DMCHECK_LT(val1, val2) MELON_DCHECK_OP(LT, < , val1, val2)
#define DMCHECK_GE(val1, val2) MELON_DCHECK_OP(GE, >=, val1, val2)
#define DMCHECK_GT(val1, val2) MELON_DCHECK_OP(GT, > , val1, val2)

#if defined(OS_WIN)
typedef unsigned long SystemErrorCode;
#elif defined(OS_POSIX)
typedef int SystemErrorCode;
#endif

// Alias for ::GetLastError() on Windows and errno on POSIX. Avoids having to
// pull in windows.h just for GetLastError() and DWORD.
MUTIL_EXPORT SystemErrorCode GetLastSystemErrorCode();
MUTIL_EXPORT void SetLastSystemErrorCode(SystemErrorCode err);
MUTIL_EXPORT std::string SystemErrorCodeToString(SystemErrorCode error_code);

// Underlying buffer to store logs. Comparing to using std::ostringstream
// directly, this utility exposes more low-level methods so that we avoid
// creation of std::string which allocates memory internally.
class CharArrayStreamBuf : public std::streambuf {
public:
    explicit CharArrayStreamBuf() : _data(NULL), _size(0) {}
    ~CharArrayStreamBuf() override;

    int overflow(int ch) override;
    int sync() override;
    void reset();

private:
    char* _data;
    size_t _size;
};

// A std::ostream to << objects.
// Have to use private inheritance to arrange initialization order.
class LogStream : virtual private CharArrayStreamBuf, public std::ostream {
friend void DestroyLogStream(LogStream*);
public:
    LogStream()
        : std::ostream(this), _file("-"), _line(0), _func("-")
        , _severity(0) , _noflush(false), _is_check(false) {
    }

    ~LogStream() {
        _noflush = false;
        Flush();
    }

    inline LogStream& operator<<(LogStream& (*m)(LogStream&)) {
        return m(*this);
    }

    inline LogStream& operator<<(std::ostream& (*m)(std::ostream&)) {
        m(*(std::ostream*)this);
        return *this;
    }

    template <typename T> inline LogStream& operator<<(T const& t) {
        *(std::ostream*)this << t;
        return *this;
    }

    // Reset the log prefix: "I0711 15:14:01.830110 12735 server.cpp:93] "
    LogStream& SetPosition(const LogChar* file, int line, LogSeverity);

    // Reset the log prefix: "E0711 15:14:01.830110 12735 server.cpp:752 StartInternal] "
    LogStream& SetPosition(const LogChar* file, int line, const LogChar* func, LogSeverity);

    // Make FlushIfNeed() no-op once.
    LogStream& DontFlushOnce() {
        _noflush = true;
        return *this;
    }

    LogStream& SetCheck() {
        _is_check = true;
        return *this;
    }

    bool empty() const { return pbase() == pptr(); }

    mutil::StringPiece content() const
    { return mutil::StringPiece(pbase(), pptr() - pbase()); }

    std::string content_str() const
    { return std::string(pbase(), pptr() - pbase()); }

    const LogChar* file() const { return _file; }
    int line() const { return _line; }
    const LogChar* func() const { return _func; }
    LogSeverity severity() const { return _severity; }

private:
    void FlushWithoutReset();

    // Flush log into sink(if registered) or stderr.
    // NOTE: make this method private to limit the callsites so that the
    // stack-frame removal in FlushWithoutReset() is always safe.
    inline void Flush() {
        const bool res = _noflush;
        _noflush = false;
        if (!res) {
            // Save and restore thread-local error code after Flush().
            const SystemErrorCode err = GetLastSystemErrorCode();
            FlushWithoutReset();
            reset();
            clear();
            SetLastSystemErrorCode(err);
            _is_check = false;
        }
    }

    const LogChar* _file;
    int _line;
    const LogChar* _func;
    LogSeverity _severity;
    bool _noflush;
    bool _is_check;
};

// This class more or less represents a particular log message.  You
// create an instance of LogMessage and then stream stuff to it.
// When you finish streaming to it, ~LogMessage is called and the
// full message gets streamed to the appropriate destination if `noflush'
// is not present.
//
// You shouldn't actually use LogMessage's constructor to log things,
// though.  You should use the MLOG() macro (and variants thereof)
// above.
class MUTIL_EXPORT LogMessage {
public:
    // Used for MLOG(severity).
    LogMessage(const char* file, int line, LogSeverity severity);
    LogMessage(const char* file, int line, const char* func,
               LogSeverity severity);

    // Used for MCHECK_EQ(), etc. Takes ownership of the given string.
    // Implied severity = BLOG_FATAL.
    LogMessage(const char* file, int line, std::string* result);
    LogMessage(const char* file, int line, const char* func,
               std::string* result);

    // Used for DMCHECK_EQ(), etc. Takes ownership of the given string.
    LogMessage(const char* file, int line, LogSeverity severity,
               std::string* result);
    LogMessage(const char* file, int line, const char* func,
               LogSeverity severity, std::string* result);

    ~LogMessage();

    LogStream& stream() { return *_stream; }

private:
    DISALLOW_COPY_AND_ASSIGN(LogMessage);

    // The real data is inside LogStream which may be cached thread-locally.
    LogStream* _stream;
};

// A non-macro interface to the log facility; (useful
// when the logging level is not a compile-time constant).
inline void LogAtLevel(int const log_level, const mutil::StringPiece &msg) {
    LogMessage(__FILE__, __LINE__, __func__,
               log_level).stream() << msg;
}

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LogMessageVoidify {
public:
    LogMessageVoidify() { }
    // This has to be an operator with a precedence lower than << but
    // higher than ?:
    void operator&(std::ostream&) { }
};

#if defined(OS_WIN)
// Appends a formatted system message of the GetLastError() type.
class MUTIL_EXPORT Win32ErrorLogMessage {
public:
    Win32ErrorLogMessage(const char* file,
                         int line,
                         LogSeverity severity,
                         SystemErrorCode err);

    Win32ErrorLogMessage(const char* file,
                         int line,
                         const char* func,
                         LogSeverity severity,
                         SystemErrorCode err);

    // Appends the error message before destructing the encapsulated class.
    ~Win32ErrorLogMessage();

    LogStream& stream() { return log_message_.stream(); }

private:
    SystemErrorCode err_;
    LogMessage log_message_;

    DISALLOW_COPY_AND_ASSIGN(Win32ErrorLogMessage);
};
#elif defined(OS_POSIX)
// Appends a formatted system message of the errno type
class MUTIL_EXPORT ErrnoLogMessage {
public:
    ErrnoLogMessage(const char* file,
                    int line,
                    LogSeverity severity,
                    SystemErrorCode err);

    ErrnoLogMessage(const char* file,
                    int line,
                    const char* func,
                    LogSeverity severity,
                    SystemErrorCode err);

    // Appends the error message before destructing the encapsulated class.
    ~ErrnoLogMessage();

    LogStream& stream() { return log_message_.stream(); }

private:
    SystemErrorCode err_;
    LogMessage log_message_;

    DISALLOW_COPY_AND_ASSIGN(ErrnoLogMessage);
};
#endif  // OS_WIN

// Closes the log file explicitly if open.
// NOTE: Since the log file is opened as necessary by the action of logging
//       statements, there's no guarantee that it will stay closed
//       after this call.
MUTIL_EXPORT void CloseLogFile();

// Async signal safe logging mechanism.
MUTIL_EXPORT void RawLog(int level, const char* message);

#define RAW_LOG(level, message)                         \
    ::logging::RawLog(::logging::BLOG_##level, message)

#define RAW_CHECK(condition, message)                                   \
    do {                                                                \
        if (!(condition))                                               \
            ::logging::RawLog(::logging::BLOG_FATAL, "Check failed: " #condition "\n"); \
    } while (0)

#if defined(OS_WIN)
// Returns the default log file path.
MUTIL_EXPORT std::wstring GetLogFileFullPath();
#endif

inline LogStream& noflush(LogStream& ls) {
    ls.DontFlushOnce();
    return ls;
}

}  // namespace logging

using ::logging::noflush;
using ::logging::VLogSitePrinter;
using ::logging::print_vlog_sites;

// These functions are provided as a convenience for logging, which is where we
// use streams (it is against Google style to use streams in other places). It
// is designed to allow you to emit non-ASCII Unicode strings to the log file,
// which is normally ASCII. It is relatively slow, so try not to use it for
// common cases. Non-ASCII characters will be converted to UTF-8 by these
// operators.
MUTIL_EXPORT std::ostream& operator<<(std::ostream& out, const wchar_t* wstr);
inline std::ostream& operator<<(std::ostream& out, const std::wstring& wstr) {
    return out << wstr.c_str();
}

// The NOTIMPLEMENTED() macro annotates codepaths which have
// not been implemented yet.
//
// The implementation of this macro is controlled by NOTIMPLEMENTED_POLICY:
//   0 -- Do nothing (stripped by compiler)
//   1 -- Warn at compile time
//   2 -- Fail at compile time
//   3 -- Fail at runtime (DMCHECK)
//   4 -- [default] MLOG(ERROR) at runtime
//   5 -- MLOG(ERROR) at runtime, only once per call-site

#endif // MELON_WITH_GLOG

#ifndef NOTIMPLEMENTED_POLICY
#if defined(OS_ANDROID) && defined(OFFICIAL_BUILD)
#define NOTIMPLEMENTED_POLICY 0
#else
// Select default policy: MLOG(ERROR)
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
#define NOTIMPLEMENTED() MELON_EAT_STREAM_PARAMS
#elif NOTIMPLEMENTED_POLICY == 1
// TODO, figure out how to generate a warning
#define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 2
#define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 3
#define NOTIMPLEMENTED() NOTREACHED()
#elif NOTIMPLEMENTED_POLICY == 4
#define NOTIMPLEMENTED() MLOG(ERROR) << NOTIMPLEMENTED_MSG
#elif NOTIMPLEMENTED_POLICY == 5
#define NOTIMPLEMENTED() do {                                   \
        static bool logged_once = false;                        \
        LOG_IF(ERROR, !logged_once) << NOTIMPLEMENTED_MSG;      \
        logged_once = true;                                     \
    } while(0);                                                 \
    MELON_EAT_STREAM_PARAMS
#endif

#if defined(NDEBUG) && defined(OS_CHROMEOS)
#define NOTREACHED() MLOG(ERROR) << "NOTREACHED() hit in "       \
    << __FUNCTION__ << ". "
#else
#define NOTREACHED() DMCHECK(false)
#endif

// Helper macro included by all *_EVERY_N macros.
#define MELON_LOG_IF_EVERY_N_IMPL(logifmacro, severity, condition, N)   \
    static ::mutil::subtle::Atomic32 MELON_CONCAT(logeveryn_, __LINE__) = -1; \
    const static int MELON_CONCAT(logeveryn_sc_, __LINE__) = (N);       \
    const int MELON_CONCAT(logeveryn_c_, __LINE__) =                    \
        ::mutil::subtle::NoBarrier_AtomicIncrement(&MELON_CONCAT(logeveryn_, __LINE__), 1); \
    logifmacro(severity, (condition) && MELON_CONCAT(logeveryn_c_, __LINE__) / \
               MELON_CONCAT(logeveryn_sc_, __LINE__) * MELON_CONCAT(logeveryn_sc_, __LINE__) \
               == MELON_CONCAT(logeveryn_c_, __LINE__))

// Helper macro included by all *_FIRST_N macros.
#define MELON_LOG_IF_FIRST_N_IMPL(logifmacro, severity, condition, N)   \
    static ::mutil::subtle::Atomic32 MELON_CONCAT(logfstn_, __LINE__) = 0; \
    logifmacro(severity, (condition) && MELON_CONCAT(logfstn_, __LINE__) < N && \
               ::mutil::subtle::NoBarrier_AtomicIncrement(&MELON_CONCAT(logfstn_, __LINE__), 1) <= N)

// Helper macro included by all *_EVERY_SECOND macros.
#define MELON_LOG_IF_EVERY_SECOND_IMPL(logifmacro, severity, condition) \
    static ::mutil::subtle::Atomic64 MELON_CONCAT(logeverys_, __LINE__) = 0; \
    const int64_t MELON_CONCAT(logeverys_ts_, __LINE__) = ::mutil::gettimeofday_us(); \
    const int64_t MELON_CONCAT(logeverys_seen_, __LINE__) = MELON_CONCAT(logeverys_, __LINE__); \
    logifmacro(severity, (condition) && MELON_CONCAT(logeverys_ts_, __LINE__) >= \
               (MELON_CONCAT(logeverys_seen_, __LINE__) + 1000000L) &&  \
               ::mutil::subtle::NoBarrier_CompareAndSwap(                \
                   &MELON_CONCAT(logeverys_, __LINE__),                 \
                   MELON_CONCAT(logeverys_seen_, __LINE__),             \
                   MELON_CONCAT(logeverys_ts_, __LINE__))               \
               == MELON_CONCAT(logeverys_seen_, __LINE__))

// ===============================================================

// Print a log for at most once. (not present in glog)
// Almost zero overhead when the log was printed.
#ifndef MLOG_ONCE
# define MLOG_ONCE(severity) MLOG_FIRST_N(severity, 1)
# define LOG_IF_ONCE(severity, condition) LOG_IF_FIRST_N(severity, condition, 1)
#endif

// Print a log after every N calls. First call always prints.
// Each call to this macro has a cost of relaxed atomic increment.
// The corresponding macro in glog is not thread-safe while this is.
#ifndef MLOG_EVERY_N
# define MLOG_EVERY_N(severity, N)                                \
     MELON_LOG_IF_EVERY_N_IMPL(LOG_IF, severity, true, N)
# define LOG_IF_EVERY_N(severity, condition, N)                  \
     MELON_LOG_IF_EVERY_N_IMPL(LOG_IF, severity, condition, N)
#endif

// Print logs for first N calls.
// Almost zero overhead when the log was printed for N times
// The corresponding macro in glog is not thread-safe while this is.
#ifndef MLOG_FIRST_N
# define MLOG_FIRST_N(severity, N)                                \
     MELON_LOG_IF_FIRST_N_IMPL(LOG_IF, severity, true, N)
# define LOG_IF_FIRST_N(severity, condition, N)                  \
     MELON_LOG_IF_FIRST_N_IMPL(LOG_IF, severity, condition, N)
#endif

// Print a log every second. (not present in glog). First call always prints.
// Each call to this macro has a cost of calling gettimeofday.
#ifndef MLOG_EVERY_SECOND
# define MLOG_EVERY_SECOND(severity)                                \
     MELON_LOG_IF_EVERY_SECOND_IMPL(LOG_IF, severity, true)
# define LOG_IF_EVERY_SECOND(severity, condition)                \
     MELON_LOG_IF_EVERY_SECOND_IMPL(LOG_IF, severity, condition)
#endif

#ifndef PMLOG_EVERY_N
# define PMLOG_EVERY_N(severity, N)                               \
     MELON_LOG_IF_EVERY_N_IMPL(PMLOG_IF, severity, true, N)
# define PLOG_IF_EVERY_N(severity, condition, N)                 \
     MELON_LOG_IF_EVERY_N_IMPL(PMLOG_IF, severity, condition, N)
#endif

#ifndef PMLOG_FIRST_N
# define PMLOG_FIRST_N(severity, N)                               \
     MELON_LOG_IF_FIRST_N_IMPL(PMLOG_IF, severity, true, N)
# define PLOG_IF_FIRST_N(severity, condition, N)                 \
     MELON_LOG_IF_FIRST_N_IMPL(PMLOG_IF, severity, condition, N)
#endif

#ifndef PMLOG_ONCE
# define PMLOG_ONCE(severity) PMLOG_FIRST_N(severity, 1)
# define PLOG_IF_ONCE(severity, condition) PLOG_IF_FIRST_N(severity, condition, 1)
#endif

#ifndef PMLOG_EVERY_SECOND
# define PMLOG_EVERY_SECOND(severity)                             \
     MELON_LOG_IF_EVERY_SECOND_IMPL(PMLOG_IF, severity, true)
# define PMLOG_IF_EVERY_SECOND(severity, condition)                       \
     MELON_LOG_IF_EVERY_SECOND_IMPL(PMLOG_IF, severity, condition)
#endif

// DEBUG_MODE is for uses like
//   if (DEBUG_MODE) foo.CheckThatFoo();
// instead of
//   #ifndef NDEBUG
//     foo.CheckThatFoo();
//   #endif
//
// We tie its state to ENABLE_DMLOG.
enum { DEBUG_MODE = DMCHECK_IS_ON() };


#endif  // MUTIL_LOGGING_H_
