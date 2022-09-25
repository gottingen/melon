
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef MELON_LOG_LOGGING_H_
#define MELON_LOG_LOGGING_H_


#include <cerrno>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <iosfwd>
#include <ostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <atomic>
#include <cstdint>
#include <atomic>

#include <gflags/gflags.h>
#include "melon/base/profile.h"
#include "melon/strings/fmt/format.h"
#include "melon/strings/fmt/ostream.h"
#include "melon/log/config.h"
#include "melon/log/severity.h"
#include "melon/log/vlog_is_on.h"
#include "melon/log/utility.h"
#include "melon/base/profile.h"
#include "melon/base/static_atomic.h" // Used by MELON_LOG_EVERY_N, MELON_LOG_FIRST_N etc
#include "melon/times/time.h"

// The global value of MELON_STRIP_LOG. All the messages logged to
// MELON_LOG(XXX) with severity less than MELON_STRIP_LOG will not be displayed.
// If it can be determined at compile time that the message will not be
// printed, the statement will be compiled out.
//
// Example: to strip out all INFO and WARNING messages, use the value
// of 2 below. To make an exception for WARNING messages from a single
// file, add "#define MELON_STRIP_LOG 1" to that file _before_ including
// logging/logging.h
#ifndef MELON_STRIP_LOG
#define MELON_STRIP_LOG 0
#endif


// Log messages below the MELON_STRIP_LOG level will be compiled away for
// security reasons. See MELON_LOG(severtiy) below.

// A few definitions of macros that don't generate much code.  Since
// MELON_LOG(INFO) and its ilk are used all over our code, it's
// better to have compact code for these operations.

#if MELON_STRIP_LOG == 0
#define COMPACT_MELON_LOG_TRACE melon::log::log_message( \
      __FILE__, __LINE__, melon::log::MELON_TRACE)
#define MELON_LOG_TO_STRING_TRACE(message) melon::log::log_message( \
      __FILE__, __LINE__, melon::log::MELON_TRACE, message)
#else
#define COMPACT_MELON_LOG_TRACE melon::log::null_stream()
#define MELON_LOG_TO_STRING_TRACE(message) melon::log::null_stream()
#endif

#if MELON_STRIP_LOG <= 1
#define COMPACT_MELON_LOG_DEBUG melon::log::log_message( \
      __FILE__, __LINE__, melon::log::MELON_DEBUG)
#define MELON_LOG_TO_STRING_DEBUG(message) melon::log::log_message( \
      __FILE__, __LINE__, melon::log::MELON_DEBUG, message)
#else
#define COMPACT_MELON_LOG_DEBUG melon::log::null_stream()
#define MELON_LOG_TO_STRING_DEBUG(message) melon::log::null_stream()
#endif

#if MELON_STRIP_LOG <= 2
#define COMPACT_MELON_LOG_INFO melon::log::log_message( \
      __FILE__, __LINE__, melon::log::MELON_INFO)
#define MELON_LOG_TO_STRING_INFO(message) melon::log::log_message( \
      __FILE__, __LINE__, melon::log::MELON_INFO, message)
#else
#define COMPACT_MELON_LOG_INFO melon::log::null_stream()
#define MELON_LOG_TO_STRING_INFO(message) melon::log::null_stream()
#endif

#if MELON_STRIP_LOG <= 3
#define COMPACT_MELON_LOG_WARNING melon::log::log_message( \
      __FILE__, __LINE__, melon::log::MELON_WARNING)
#define MELON_LOG_TO_STRING_WARNING(message) melon::log::log_message( \
      __FILE__, __LINE__, melon::log::MELON_WARNING, message)
#else
#define COMPACT_MELON_LOG_WARNING melon::log::null_stream()
#define MELON_LOG_TO_STRING_WARNING(message) melon::log::null_stream()
#endif

#if MELON_STRIP_LOG <= 4
#define COMPACT_MELON_LOG_ERROR melon::log::log_message( \
      __FILE__, __LINE__, melon::log::MELON_ERROR)
#define MELON_LOG_TO_STRING_ERROR(message) melon::log::log_message( \
      __FILE__, __LINE__, melon::log::MELON_ERROR, message)
#else
#define COMPACT_MELON_LOG_ERROR melon::log::null_stream()
#define MELON_LOG_TO_STRING_ERROR(message) melon::log::null_stream()
#endif

#if MELON_STRIP_LOG <= 5
#define COMPACT_MELON_LOG_FATAL melon::log::log_message_fatal( \
      __FILE__, __LINE__)
#define MELON_LOG_TO_STRING_FATAL(message) melon::log::log_message( \
      __FILE__, __LINE__, melon::log::MELON_FATAL, message)
#else
#define COMPACT_MELON_LOG_FATAL melon::log::null_stream_fatal()
#define MELON_LOG_TO_STRING_FATAL(message) melon::log::null_stream_fatal()
#endif

#ifndef MELON_DCHECK_IS_ON
#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define MELON_DCHECK_IS_ON() 0
#else
#define MELON_DCHECK_IS_ON() 1
#endif
#endif

// For DFATAL, we want to use log_message (as opposed to
// log_message_fatal), to be consistent with the original behavior.
#if !MELON_DCHECK_IS_ON()
#define COMPACT_MELON_LOG_DFATAL COMPACT_MELON_LOG_ERROR
#elif MELON_STRIP_LOG <= 3
#define COMPACT_MELON_LOG_DFATAL melon::log::log_message( \
      __FILE__, __LINE__, melon::log::MELON_FATAL)
#else
#define COMPACT_MELON_LOG_DFATAL melon::log::null_stream_fatal()
#endif

#define MELON_SYSLOG_TRACE(counter) \
  melon::log::log_message(__FILE__, __LINE__, melon::log::MELON_TRACE, counter, \
  &melon::log::log_message::send_to_syslog_and_log)
#define MELON_SYSLOG_DEBUG(counter) \
  melon::log::log_message(__FILE__, __LINE__, melon::log::MELON_DEBUG, counter, \
  &melon::log::log_message::send_to_syslog_and_log)
#define MELON_SYSLOG_INFO(counter) \
  melon::log::log_message(__FILE__, __LINE__, melon::log::MELON_INFO, counter, \
  &melon::log::log_message::send_to_syslog_and_log)
#define MELON_SYSLOG_WARNING(counter)  \
  melon::log::log_message(__FILE__, __LINE__, melon::log::MELON_WARNING, counter, \
  &melon::log::log_message::send_to_syslog_and_log)
#define MELON_SYSLOG_ERROR(counter)  \
  melon::log::log_message(__FILE__, __LINE__, melon::log::MELON_ERROR, counter, \
  &melon::log::log_message::send_to_syslog_and_log)
#define MELON_SYSLOG_FATAL(counter) \
  melon::log::log_message(__FILE__, __LINE__, melon::log::MELON_FATAL, counter, \
  &melon::log::log_message::send_to_syslog_and_log)
#define MELON_SYSLOG_DFATAL(counter) \
  melon::log::log_message(__FILE__, __LINE__, melon::log::DFATAL_LEVEL, counter, \
  &melon::log::log_message::send_to_syslog_and_log)

// We use the preprocessor's merging operator, "##", so that, e.g.,
// MELON_LOG(INFO) becomes the token LOG_INFO.  There's some funny
// subtle difference between ostream member streaming functions (e.g.,
// ostream::operator<<(int) and ostream non-member streaming functions
// (e.g., ::operator<<(ostream&, string&): it turns out that it's
// impossible to stream something like a string directly to an unnamed
// ostream. We employ a neat hack by calling the stream() member
// function of log_message which seems to avoid the problem.
#define MELON_LOG(severity) COMPACT_MELON_LOG_ ## severity.stream()
#define MELON_SYSLOG(severity) MELON_SYSLOG_ ## severity(0).stream()

namespace melon::log {

    // They need the definitions of integer types.

    // Initialize melon::log's logging library. You will see the program name
    // specified by argv0 in log outputs.
    MELON_EXPORT void init_logging(const char *argv0);

    // Shutdown melon::log's logging library.
    MELON_EXPORT void shutdown_logging();

    // Install a function which will be called after MELON_LOG(FATAL).
    MELON_EXPORT void InstallFailureFunction(void (*fail_func)());

    // Enable/Disable old log cleaner.
    MELON_EXPORT void enable_log_cleaner(int overdue_days);

    MELON_EXPORT void disable_log_cleaner();

    MELON_EXPORT void SetApplicationFingerprint(const std::string &fingerprint);

    class log_sink;  // defined below

    // If a non-NULL sink pointer is given, we push this message to that sink.
    // For MELON_LOG_TO_SINK we then do normal MELON_LOG(severity) logging as well.
    // This is useful for capturing messages and passing/storing them
    // somewhere more specific than the global log of the process.
    // Argument types:
    //   log_sink* sink;
    //   log_severity severity;
    // The cast is to disambiguate NULL arguments.
#define MELON_LOG_TO_SINK(sink, severity) \
  melon::log::log_message(                                    \
      __FILE__, __LINE__,                                               \
      melon::log::MELON_ ## severity,                         \
      static_cast<melon::log::log_sink*>(sink), true).stream()
#define MELON_LOG_TO_SINK_BUT_NOT_TO_LOGFILE(sink, severity)                  \
  melon::log::log_message(                                    \
      __FILE__, __LINE__,                                               \
      melon::log::MELON_ ## severity,                         \
      static_cast<melon::log::log_sink*>(sink), false).stream()

// If a non-NULL string pointer is given, we write this message to that string.
// We then do normal MELON_LOG(severity) logging as well.
// This is useful for capturing messages and storing them somewhere more
// specific than the global log of the process.
// Argument types:
//   string* message;
//   log_severity severity;
// The cast is to disambiguate NULL arguments.
// NOTE: MELON_LOG(severity) expands to log_message().stream() for the specified
// severity.
#define MELON_LOG_TO_STRING(severity, message) \
  MELON_LOG_TO_STRING_##severity(static_cast<std::string*>(message)).stream()

// If a non-NULL pointer is given, we push the message onto the end
// of a vector of strings; otherwise, we report it with MELON_LOG(severity).
// This is handy for capturing messages and perhaps passing them back
// to the caller, rather than reporting them immediately.
// Argument types:
//   log_severity severity;
//   vector<string> *outvec;
// The cast is to disambiguate NULL arguments.
#define MELON_LOG_STRING(severity, outvec) \
  MELON_LOG_TO_STRING_##severity(static_cast<std::vector<std::string>*>(outvec)).stream()

#define MELON_LOG_IF(severity, condition) \
  static_cast<void>(0),             \
  !(condition) ? (void) 0 : melon::log::log_message_voidify() & MELON_LOG(severity)
#define MELON_SYSLOG_IF(severity, condition) \
  static_cast<void>(0),                \
  !(condition) ? (void) 0 : melon::log::log_message_voidify() & MELON_SYSLOG(severity)

#define MELON_LOG_ASSERT(condition)  \
  MELON_LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition
#define MELON_SYSLOG_ASSERT(condition) \
  MELON_SYSLOG_IF(FATAL, !(condition)) << "Assert failed: " #condition

// MELON_CHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by MELON_DCHECK_IS_ON(), so the check will be executed regardless of
// compilation mode.  Therefore, it is safe to do things like:
//    MELON_CHECK(fp->Write(x) == 4)
#define MELON_CHECK(condition)  \
      MELON_LOG_IF(FATAL, MELON_UNLIKELY(!(condition))) \
             << "Check failed: " #condition " "

    // A container for a string pointer which can be evaluated to a bool -
    // true iff the pointer is NULL.
    struct CheckOpString {
        CheckOpString(std::string *str) : str_(str) {}

        // No destructor: if str_ is non-NULL, we're about to MELON_LOG(FATAL),
        // so there's no point in cleaning up str_.
        operator bool() const {
            return MELON_UNLIKELY(str_ != NULL);
        }

        std::string *str_;
    };

    // Function is overloaded for integral types to allow static const
    // integrals declared in classes and not defined to be used as arguments to
    // MELON_CHECK* macros. It's not encouraged though.
    template<class T>
    inline const T &get_referenceable_value(const T &t) { return t; }

    inline char get_referenceable_value(char t) { return t; }

    inline unsigned char get_referenceable_value(unsigned char t) { return t; }

    inline signed char get_referenceable_value(signed char t) { return t; }

    inline short get_referenceable_value(short t) { return t; }

    inline unsigned short get_referenceable_value(unsigned short t) { return t; }

    inline int get_referenceable_value(int t) { return t; }

    inline unsigned int get_referenceable_value(unsigned int t) { return t; }

    inline long get_referenceable_value(long t) { return t; }

    inline unsigned long get_referenceable_value(unsigned long t) { return t; }

    inline long long get_referenceable_value(long long t) { return t; }

    inline unsigned long long get_referenceable_value(unsigned long long t) {
        return t;
    }

    // This is a dummy class to define the following operator.
    struct dummy_class_to_define_operator {
    };

}  // namespace melon::log

// Define global operator<< to declare using ::operator<<.
// This declaration will allow use to use MELON_CHECK macros for user
// defined classes which have operator<< (e.g., stl_logging.h).
inline std::ostream &operator<<(
        std::ostream &out, const melon::log::dummy_class_to_define_operator &) {
    return out;
}

namespace melon::log {

    // This formats a value for a failing CHECK_XX statement.  Ordinarily,
    // it uses the definition for operator<<, with a few special cases below.
    template<typename T>
    inline void make_check_op_value_string(std::ostream *os, const T &v) {
        (*os) << v;
    }

    // Overrides for char types provide readable values for unprintable
    // characters.
    template<>
    MELON_EXPORT
    void make_check_op_value_string(std::ostream *os, const char &v);

    template<>
    MELON_EXPORT
    void make_check_op_value_string(std::ostream *os, const signed char &v);

    template<>
    MELON_EXPORT
    void make_check_op_value_string(std::ostream *os, const unsigned char &v);

    // This is required because nullptr is only present in c++ 11 and later.
    // Provide printable value for nullptr_t
    template<>
    MELON_EXPORT
    void make_check_op_value_string(std::ostream *os, const std::nullptr_t &v);

    // Build the error message string. Specify no inlining for code size.
    template<typename T1, typename T2>
    MELON_NO_INLINE std::string *MakeCheckOpString(const T1 &v1, const T2 &v2, const char *exprtext);


    namespace base {
        namespace internal {

            // If "s" is less than base_logging::INFO, returns base_logging::INFO.
            // If "s" is greater than base_logging::FATAL, returns
            // base_logging::ERROR.  Otherwise, returns "s".
            log_severity NormalizeSeverity(log_severity s);

        }  // namespace internal

        // A helper class for formatting "expr (V1 vs. V2)" in a CHECK_XX
        // statement.  See MakeCheckOpString for sample usage.  Other
        // approaches were considered: use of a template method (e.g.,
        // base::BuildCheckOpString(exprtext, base::Print<T1>, &v1,
        // base::Print<T2>, &v2), however this approach has complications
        // related to volatile arguments and function-pointer arguments).
        class MELON_EXPORT CheckOpMessageBuilder {
        public:
            // Inserts "exprtext" and " (" to the stream.
            explicit CheckOpMessageBuilder(const char *exprtext);

            // Deletes "stream_".
            ~CheckOpMessageBuilder();

            // For inserting the first variable.
            std::ostream *ForVar1() { return stream_; }

            // For inserting the second variable (adds an intermediate " vs. ").
            std::ostream *ForVar2();

            // Get the result (inserts the closing ")").
            std::string *NewString();

        private:
            std::ostringstream *stream_;
        };

    }  // namespace base

    template<typename T1, typename T2>
    std::string *MakeCheckOpString(const T1 &v1, const T2 &v2, const char *exprtext) {
        base::CheckOpMessageBuilder comb(exprtext);
        make_check_op_value_string(comb.ForVar1(), v1);
        make_check_op_value_string(comb.ForVar2(), v2);
        return comb.NewString();
    }

// Helper functions for CHECK_OP macro.
// The (int, int) specialization works around the issue that the compiler
// will not instantiate the template version of the function on values of
// unnamed enum type - see comment below.
#define MELON_DEFINE_CHECK_OP_IMPL(name, op) \
  template <typename T1, typename T2> \
  inline std::string* name##Impl(const T1& v1, const T2& v2,    \
                            const char* exprtext) { \
    if (MELON_LIKELY(v1 op v2)) return NULL; \
    else return MakeCheckOpString(v1, v2, exprtext); \
  } \
  inline std::string* name##Impl(int v1, int v2, const char* exprtext) { \
    return name##Impl<int, int>(v1, v2, exprtext); \
  }

    // We use the full name Check_EQ, Check_NE, etc. in case the file including
    // base/logging.h provides its own #defines for the simpler names EQ, NE, etc.
    // This happens if, for example, those are used as token names in a
    // yacc grammar.
    MELON_DEFINE_CHECK_OP_IMPL(Check_EQ, ==)  // Compilation error with MELON_CHECK_EQ(NULL, x)?
    MELON_DEFINE_CHECK_OP_IMPL(Check_NE, !=)  // Use MELON_CHECK(x == NULL) instead.
    MELON_DEFINE_CHECK_OP_IMPL(Check_LE, <=)

    MELON_DEFINE_CHECK_OP_IMPL(Check_LT, <)

    MELON_DEFINE_CHECK_OP_IMPL(Check_GE, >=)

    MELON_DEFINE_CHECK_OP_IMPL(Check_GT, >)

#undef MELON_DEFINE_CHECK_OP_IMPL

// Helper macro for binary operators.
// Don't use this macro directly in your code, use MELON_CHECK_EQ et al below.

#if defined(STATIC_ANALYSIS)
    // Only for static analysis tool to know that it is equivalent to assert
#define MELON_CHECK_OP_LOG(name, op, val1, val2, log) MELON_CHECK((val1) op (val2))
#elif MELON_DCHECK_IS_ON()
    // In debug mode, avoid constructing CheckOpStrings if possible,
    // to reduce the overhead of MELON_CHECK statments by 2x.
    // Real MELON_DCHECK-heavy tests have seen 1.5x speedups.

    // The meaning of "string" might be different between now and
    // when this macro gets invoked (e.g., if someone is experimenting
    // with other string implementations that get defined after this
    // file is included).  Save the current meaning now and use it
    // in the macro.
    typedef std::string _Check_string;
#define MELON_CHECK_OP_LOG(name, op, val1, val2, log_)                         \
  while (melon::log::_Check_string* _result =                \
        melon::log::Check##name##Impl(                      \
             melon::log::get_referenceable_value(val1),        \
             melon::log::get_referenceable_value(val2),        \
#val1 " " #op " " #val2))                                  \
    log_(__FILE__, __LINE__,                                             \
        melon::log::CheckOpString(_result)).stream()
#else
    // In optimized mode, use CheckOpString to hint to compiler that
    // the while condition is unlikely.
#define MELON_CHECK_OP_LOG(name, op, val1, val2, log_)                         \
  while (melon::log::CheckOpString _result =                 \
         melon::log::Check##name##Impl(                      \
             melon::log::get_referenceable_value(val1),        \
             melon::log::get_referenceable_value(val2),        \
#val1 " " #op " " #val2))                                  \
    log_(__FILE__, __LINE__, _result).stream()
#endif  // STATIC_ANALYSIS, MELON_DCHECK_IS_ON()

#if MELON_STRIP_LOG <= 3
#define MELON_CHECK_OP(name, op, val1, val2) \
  MELON_CHECK_OP_LOG(name, op, val1, val2, melon::log::log_message_fatal)
#else
#define CHECK_OP(name, op, val1, val2) \
  MELON_CHECK_OP_LOG(name, op, val1, val2, melon::log::null_stream_fatal)
#endif // STRIP_LOG <= 3

// Equality/Inequality checks - compare two values, and log a FATAL message
// including the two values when the result is not as expected.  The values
// must have operator<<(ostream, ...) defined.
//
// You may append to the error message like so:
//   MELON_CHECK_NE(1, 2) << ": The world must be ending!";
//
// We are very careful to ensure that each argument is evaluated exactly
// once, and that anything which is legal to pass as a function argument is
// legal here.  In particular, the arguments may be temporary expressions
// which will end up being destroyed at the end of the apparent statement,
// for example:
//   MELON_CHECK_EQ(string("abc")[1], 'b');
//
// WARNING: These don't compile correctly if one of the arguments is a pointer
// and the other is NULL. To work around this, simply static_cast NULL to the
// type of the desired pointer.

#define MELON_CHECK_EQ(val1, val2) MELON_CHECK_OP(_EQ, ==, val1, val2)
#define MELON_CHECK_NE(val1, val2) MELON_CHECK_OP(_NE, !=, val1, val2)
#define MELON_CHECK_LE(val1, val2) MELON_CHECK_OP(_LE, <=, val1, val2)
#define MELON_CHECK_LT(val1, val2) MELON_CHECK_OP(_LT, < , val1, val2)
#define MELON_CHECK_GE(val1, val2) MELON_CHECK_OP(_GE, >=, val1, val2)
#define MELON_CHECK_GT(val1, val2) MELON_CHECK_OP(_GT, > , val1, val2)

// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define MELON_CHECK_NOTNULL(val) \
  melon::log::check_not_null(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// Helper functions for string comparisons.
// To avoid bloat, the definitions are in logging.cc.
#define MELON_DECLARE_CHECK_STROP_IMPL(func, expected) \
  MELON_EXPORT std::string* Check##func##expected##Impl( \
      const char* s1, const char* s2, const char* names);

    MELON_DECLARE_CHECK_STROP_IMPL(strcmp, true)

    MELON_DECLARE_CHECK_STROP_IMPL(strcmp, false)

    MELON_DECLARE_CHECK_STROP_IMPL(strcasecmp, true)

    MELON_DECLARE_CHECK_STROP_IMPL(strcasecmp, false)

#undef MELON_DECLARE_CHECK_STROP_IMPL

// Helper macro for string comparisons.
// Don't use this macro directly in your code, use MELON_CHECK_STREQ et al below.
#define MELON_CHECK_STROP(func, op, expected, s1, s2) \
  while (melon::log::CheckOpString _result = \
         melon::log::Check##func##expected##Impl((s1), (s2), \
                                     #s1 " " #op " " #s2)) \
    MELON_LOG(FATAL) << *_result.str_


// String (char*) equality/inequality checks.
// CASE versions are case-insensitive.
//
// Note that "s1" and "s2" may be temporary strings which are destroyed
// by the compiler at the end of the current "full expression"
// (e.g. MELON_CHECK_STREQ(Foo().c_str(), Bar().c_str())).

#define MELON_CHECK_STREQ(s1, s2) MELON_CHECK_STROP(strcmp, ==, true, s1, s2)
#define MELON_CHECK_STRNE(s1, s2) MELON_CHECK_STROP(strcmp, !=, false, s1, s2)
#define MELON_CHECK_STRCASEEQ(s1, s2) MELON_CHECK_STROP(strcasecmp, ==, true, s1, s2)
#define MELON_CHECK_STRCASENE(s1, s2) MELON_CHECK_STROP(strcasecmp, !=, false, s1, s2)

#define MELON_CHECK_INDEX(I, A) MELON_CHECK(I < (sizeof(A)/sizeof(A[0])))
#define MELON_CHECK_BOUND(B, A) MELON_CHECK(B <= (sizeof(A)/sizeof(A[0])))

#define MELON_CHECK_DOUBLE_EQ(val1, val2)              \
  do {                                           \
    MELON_CHECK_LE((val1), (val2)+0.000000000000001L); \
    MELON_CHECK_GE((val1), (val2)-0.000000000000001L); \
  } while (0)

#define MELON_CHECK_NEAR(val1, val2, margin)           \
  do {                                           \
    MELON_CHECK_LE((val1), (val2)+(margin));           \
    MELON_CHECK_GE((val1), (val2)-(margin));           \
  } while (0)

// perror()..googly style!
//
// MELON_PLOG() and MELON_PLOG_IF() and MELON_PCHECK() behave exactly like their MELON_LOG* and
// MELON_CHECK equivalents with the addition that they postpend a description
// of the current state of errno to their output lines.

#define MELON_PLOG(severity) PLOG_IMPL(severity, 0).stream()

#define PLOG_IMPL(severity, counter)  \
  melon::log::errno_log_message( \
      __FILE__, __LINE__, melon::log::MELON_ ## severity, counter, \
      &melon::log::log_message::send_to_log)

#define MELON_PLOG_IF(severity, condition) \
  static_cast<void>(0),              \
  !(condition) ? (void) 0 : melon::log::log_message_voidify() & MELON_PLOG(severity)

// A MELON_CHECK() macro that postpends errno if the condition is false. E.g.
//
// if (poll(fds, nfds, timeout) == -1) { MELON_PCHECK(errno == EINTR); ... }
#define MELON_PCHECK(condition)  \
      MELON_PLOG_IF(FATAL, MELON_UNLIKELY(!(condition))) \
              << "Check failed: " #condition " "

// A MELON_CHECK() macro that lets you assert the success of a function that
// returns -1 and sets errno in case of an error. E.g.
//
// MELON_CHECK_ERR(mkdir(path, 0700));
//
// or
//
// int fd = open(filename, flags); MELON_CHECK_ERR(fd) << ": open " << filename;
#define MELON_CHECK_ERR(invocation)                                          \
MELON_PLOG_IF(FATAL, MELON_UNLIKELY((invocation) == -1))    \
        << #invocation

// Use macro expansion to create, for each use of MELON_LOG_EVERY_N(), static
// variables with the __LINE__ expansion as part of the variable name.
#define LOG_EVERY_N_VARNAME(base, line) LOG_EVERY_N_VARNAME_CONCAT(base, line)
#define LOG_EVERY_N_VARNAME_CONCAT(base, line) base ## line

#define LOG_OCCURRENCES LOG_EVERY_N_VARNAME(occurrences_, __LINE__)
#define LOG_OCCURRENCES_MOD_N LOG_EVERY_N_VARNAME(occurrences_mod_n_, __LINE__)

#if MELON_COMPILER_HAS_FEATURE(thread_sanitizer) || (defined(__SANITIZE_THREAD__) && __SANITIZE_THREAD__)
#define MELON_SANITIZE_THREAD 1
#endif


#if defined(MELON_SANITIZE_THREAD)
    } // namespace melon::log

    // We need to identify the static variables as "benign" races
    // to avoid noisy reports from TSAN.
    extern "C" void AnnotateBenignRaceSized(
      const char *file,
      int line,
      const volatile void *mem,
      long size,
      const char *description);

    namespace melon::log {
#endif

#define MELON_SOME_KIND_OF_LOG_EVERY_N(severity, n, what_to_do) \
  static std::atomic<int> LOG_OCCURRENCES(0), LOG_OCCURRENCES_MOD_N(0); \
  MELON_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES, sizeof(int), "")); \
  MELON_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES_MOD_N, sizeof(int), "")); \
  ++LOG_OCCURRENCES; \
  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
  if (LOG_OCCURRENCES_MOD_N == 1) \
    melon::log::log_message( \
        __FILE__, __LINE__, melon::log::MELON_ ## severity, LOG_OCCURRENCES, \
        &what_to_do).stream()

#define MELON_SOME_KIND_OF_LOG_IF_EVERY_N(severity, condition, n, what_to_do) \
  static std::atomic<int> LOG_OCCURRENCES(0), LOG_OCCURRENCES_MOD_N(0); \
  MELON_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES, sizeof(int), "")); \
  MELON_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES_MOD_N, sizeof(int), "")); \
  ++LOG_OCCURRENCES; \
  if (condition && \
      ((LOG_OCCURRENCES_MOD_N=(LOG_OCCURRENCES_MOD_N + 1) % n) == (1 % n))) \
    melon::log::log_message( \
        __FILE__, __LINE__, melon::log::MELON_ ## severity, LOG_OCCURRENCES, \
                 &what_to_do).stream()

#define MELON_SOME_KIND_OF_PLOG_EVERY_N(severity, n, what_to_do) \
  static std::atomic<int> LOG_OCCURRENCES(0), LOG_OCCURRENCES_MOD_N(0); \
  MELON_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES, sizeof(int), "")); \
  MELON_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES_MOD_N, sizeof(int), "")); \
  ++LOG_OCCURRENCES; \
  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
  if (LOG_OCCURRENCES_MOD_N == 1) \
    melon::log::errno_log_message( \
        __FILE__, __LINE__, melon::log::MELON_ ## severity, LOG_OCCURRENCES, \
        &what_to_do).stream()

#define MELON_SOME_KIND_OF_LOG_FIRST_N(severity, n, what_to_do) \
  static std::atomic<int> LOG_OCCURRENCES(0); \
  MELON_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES, sizeof(int), "")); \
  if (LOG_OCCURRENCES <= n) \
    ++LOG_OCCURRENCES; \
  if (LOG_OCCURRENCES <= n) \
    melon::log::log_message( \
        __FILE__, __LINE__, melon::log::MELON_ ## severity, LOG_OCCURRENCES, \
        &what_to_do).stream()

    namespace log_internal {
        template<bool>
        struct compile_assert {
        };
        struct crash_reason;

        // Returns true if FailureSignalHandler is installed.
        // Needs to be exported since it's used by the signalhandler_unittest.
        MELON_EXPORT bool IsFailureSignalHandlerInstalled();
    }  // namespace log_internal

#define MELON_LOG_EVERY_N(severity, n)                                        \
  MELON_SOME_KIND_OF_LOG_EVERY_N(severity, (n), melon::log::log_message::send_to_log)

#define MELON_SYSLOG_EVERY_N(severity, n) \
  MELON_SOME_KIND_OF_LOG_EVERY_N(severity, (n), melon::log::log_message::send_to_syslog_and_log)

#define MELON_PLOG_EVERY_N(severity, n) \
  MELON_SOME_KIND_OF_PLOG_EVERY_N(severity, (n), melon::log::log_message::send_to_log)

#define MELON_LOG_FIRST_N(severity, n) \
  MELON_SOME_KIND_OF_LOG_FIRST_N(severity, (n), melon::log::log_message::send_to_log)

#define MELON_LOG_IF_EVERY_N(severity, condition, n) \
  MELON_SOME_KIND_OF_LOG_IF_EVERY_N(severity, (condition), (n), melon::log::log_message::send_to_log)

// We want the special COUNTER value available for LOG_EVERY_X()'ed messages
    enum PRIVATE_Counter {
        COUNTER
    };

#ifdef MELON_LOG_NO_ABBREVIATED_SEVERITIES
    // wingdi.h defines ERROR to be 0. When we call MELON_LOG(ERROR), it gets
    // substituted with 0, and it expands to COMPACT_MELON_LOG_0. To allow us
    // to keep using this syntax, we define this macro to do the same thing
    // as COMPACT_MELON_LOG_ERROR.
#define COMPACT_MELON_LOG_0 COMPACT_MELON_LOG_ERROR
#define MELON_SYSLOG_0 MELON_SYSLOG_ERROR
#define MELON_LOG_TO_STRING_0 LOG_TO_STRING_ERROR
    // Needed for LOG_IS_ON(ERROR).
    const log_severity MELON_LOG_0 = MELON_ERROR;
#else
// Users may include windows.h after logging.h without
// MELON_LOG_NO_ABBREVIATED_SEVERITIES nor WIN32_LEAN_AND_MEAN.
// For this case, we cannot detect if ERROR is defined before users
// actually use ERROR. Let's make an undefined symbol to warn users.
# define MELON_LOG_ERROR_MSG ERROR_macro_is_defined_Define_MELON_LOG_NO_ABBREVIATED_SEVERITIES_before_including_logging_h_See_the_document_for_detail
# define COMPACT_MELON_LOG_0 MELON_LOG_ERROR_MSG
# define MELON_SYSLOG_0 MELON_LOG_ERROR_MSG
# define MELON_LOG_TO_STRING_0 MELON_LOG_ERROR_MSG
# define MELON_LOG_0 MELON_LOG_ERROR_MSG
#endif

// Plus some debug-logging macros that get compiled to nothing for production

#if MELON_DCHECK_IS_ON()

#define MELON_DLOG(severity) MELON_LOG(severity)
#define MELON_DVLOG(verboselevel) MELON_VLOG(verboselevel)
#define MELON_DLOG_IF(severity, condition) MELON_LOG_IF(severity, condition)
#define MELON_DLOG_EVERY_N(severity, n) MELON_LOG_EVERY_N(severity, n)
#define MELON_DLOG_IF_EVERY_N(severity, condition, n) \
  MELON_LOG_IF_EVERY_N(severity, condition, n)
#define MELON_DLOG_ASSERT(condition) MELON_LOG_ASSERT(condition)

    // debug-only checking.  executed if MELON_DCHECK_IS_ON().
#define MELON_DCHECK(condition) MELON_CHECK(condition)
#define MELON_DCHECK_EQ(val1, val2) MELON_CHECK_EQ(val1, val2)
#define MELON_DCHECK_NE(val1, val2) MELON_CHECK_NE(val1, val2)
#define MELON_DCHECK_LE(val1, val2) MELON_CHECK_LE(val1, val2)
#define MELON_DCHECK_LT(val1, val2) MELON_CHECK_LT(val1, val2)
#define MELON_DCHECK_GE(val1, val2) MELON_CHECK_GE(val1, val2)
#define MELON_DCHECK_GT(val1, val2) MELON_CHECK_GT(val1, val2)
#define MELON_DCHECK_NOTNULL(val) MELON_CHECK_NOTNULL(val)
#define MELON_DCHECK_STREQ(str1, str2) MELON_CHECK_STREQ(str1, str2)
#define MELON_DCHECK_STRCASEEQ(str1, str2) MELON_CHECK_STRCASEEQ(str1, str2)
#define MELON_DCHECK_STRNE(str1, str2) MELON_CHECK_STRNE(str1, str2)
#define MELON_DCHECK_STRCASENE(str1, str2) MELON_CHECK_STRCASENE(str1, str2)

#else  // !MELON_DCHECK_IS_ON()

#define MELON_DLOG(severity)  \
  static_cast<void>(0), \
  true ? (void) 0 : melon::log::log_message_voidify() & MELON_LOG(severity)

#define MELON_DVLOG(verboselevel)             \
  static_cast<void>(0),                 \
  (true || !MELON_VLOG_IS_ON(verboselevel)) ? \
      (void) 0 : melon::log::log_message_voidify() & MELON_LOG(INFO)

#define MELON_DLOG_IF(severity, condition) \
  static_cast<void>(0),              \
  (true || !(condition)) ? (void) 0 : melon::log::log_message_voidify() & MELON_LOG(severity)

#define MELON_DLOG_EVERY_N(severity, n) \
  static_cast<void>(0),           \
  true ? (void) 0 : melon::log::log_message_voidify() & MELON_LOG(severity)

#define MELON_DLOG_IF_EVERY_N(severity, condition, n) \
  static_cast<void>(0),                         \
  (true || !(condition))? (void) 0 : melon::log::log_message_voidify() & MELON_LOG(severity)

#define MELON_DLOG_ASSERT(condition) \
  static_cast<void>(0),        \
  true ? (void) 0 : MELON_LOG_ASSERT(condition)

    // MSVC warning C4127: conditional expression is constant
#define MELON_DCHECK(condition) \
  while (false) \
    MELON_CHECK(condition)

#define MELON_DCHECK_EQ(val1, val2) \
  while (false) \
    MELON_CHECK_EQ(val1, val2)

#define MELON_DCHECK_NE(val1, val2) \
  while (false) \
    MELON_CHECK_NE(val1, val2)

#define MELON_DCHECK_LE(val1, val2) \
  while (false) \
    MELON_CHECK_LE(val1, val2)

#define MELON_DCHECK_LT(val1, val2) \
  while (false) \
    MELON_CHECK_LT(val1, val2)

#define MELON_DCHECK_GE(val1, val2) \
  while (false) \
    MELON_CHECK_GE(val1, val2)

#define MELON_DCHECK_GT(val1, val2) \
  while (false) \
    MELON_CHECK_GT(val1, val2)

    // You may see warnings in release mode if you don't use the return
    // value of MELON_DCHECK_NOTNULL. Please just use MELON_DCHECK for such cases.
#define MELON_DCHECK_NOTNULL(val) (val)

#define MELON_DCHECK_STREQ(str1, str2) \
  while (false) \
    MELON_CHECK_STREQ(str1, str2)

#define MELON_DCHECK_STRCASEEQ(str1, str2) \
  while (false) \
    MELON_CHECK_STRCASEEQ(str1, str2)

#define MELON_DCHECK_STRNE(str1, str2) \
  while (false) \
    MELON_CHECK_STRNE(str1, str2)

#define MELON_DCHECK_STRCASENE(str1, str2) \
  while (false) \
    CHECK_STRCASENE(str1, str2)

#endif  // MELON_DCHECK_IS_ON()

// Log only in verbose mode.

#define MELON_VLOG(verboselevel) MELON_LOG_IF(INFO, MELON_VLOG_IS_ON(verboselevel))

#define MELON_VLOG_IF(verboselevel, condition) \
  MELON_LOG_IF(INFO, (condition) && MELON_VLOG_IS_ON(verboselevel))

#define MELON_VLOG_EVERY_N(verboselevel, n) \
  MELON_LOG_IF_EVERY_N(INFO, MELON_VLOG_IS_ON(verboselevel), n)

#define MELON_VLOG_IF_EVERY_N(verboselevel, condition, n) \
  MELON_LOG_IF_EVERY_N(INFO, (condition) && MELON_VLOG_IS_ON(verboselevel), n)

    namespace base_logging {

        // log_message::log_stream is a std::ostream backed by this streambuf.
        // This class ignores overflow and leaves two bytes at the end of the
        // buffer to allow for a '\n' and '\0'.
        class MELON_EXPORT LogStreamBuf : public std::streambuf {
        public:
            // REQUIREMENTS: "len" must be >= 2 to account for the '\n' and '\0'.
            LogStreamBuf(char *buf, int len) {
                setp(buf, buf + len - 2);
            }

            // This effectively ignores overflow.
            int_type overflow(int_type ch) {
                return ch;
            }

            // Legacy public ostrstream method.
            size_t pcount() const { return pptr() - pbase(); }

            char *pbase() const { return std::streambuf::pbase(); }
        };

    }  // namespace base_logging

    //
    // This class more or less represents a particular log message.  You
    // create an instance of log_message and then stream stuff to it.
    // When you finish streaming to it, ~log_message is called and the
    // full message gets streamed to the appropriate destination.
    //
    // You shouldn't actually use log_message's constructor to log things,
    // though.  You should use the MELON_LOG() macro (and variants thereof)
    // above.
    class MELON_EXPORT log_message {
    public:
        enum {
            // Passing kNoLogPrefix for the line number disables the
            // log-message prefix. Useful for using the log_message
            // infrastructure as a printing utility. See also the --melon_log_prefix
            // flag for controlling the log-message prefix on an
            // application-wide basis.
            kNoLogPrefix = -1
        };

        // log_stream inherit from non-DLL-exported class (std::ostrstream)
        // and VC++ produces a warning for this situation.
        // However, MSDN says "C4275 can be ignored in Microsoft Visual C++
        // 2005 if you are deriving from a type in the Standard C++ Library"
        // http://msdn.microsoft.com/en-us/library/3tdb471s(VS.80).aspx
        // Let's just ignore the warning.
        class MELON_EXPORT log_stream : public std::ostream {
        public:
            log_stream(char *buf, int len, uint64_t ctr)
                    : std::ostream(NULL),
                      streambuf_(buf, len),
                      ctr_(ctr),
                      self_(this) {
                rdbuf(&streambuf_);
            }

            uint64_t ctr() const { return ctr_; }

            void set_ctr(uint64_t ctr) { ctr_ = ctr; }

            log_stream *self() const { return self_; }

            // Legacy std::streambuf methods.
            size_t pcount() const { return streambuf_.pcount(); }

            char *pbase() const { return streambuf_.pbase(); }

            char *str() const { return pbase(); }

        private:

            log_stream(const log_stream &);

            log_stream &operator=(const log_stream &);

            base_logging::LogStreamBuf streambuf_;
            uint64_t ctr_;  // Counter hack (for the LOG_EVERY_X() macro)
            log_stream *self_;  // Consistency check hack
        };

    public:
        // icc 8 requires this typedef to avoid an internal compiler error.
        typedef void (log_message::*SendMethod)();

        log_message(const char *file, int line, log_severity severity, uint64_t ctr,
                    SendMethod send_method);

        // Two special constructors that generate reduced amounts of code at
        // MELON_LOG call sites for common cases.

        // Used for MELON_LOG(INFO): Implied are:
        // severity = INFO, ctr = 0, send_method = &log_message::send_to_log.
        //
        // Using this constructor instead of the more complex constructor above
        // saves 19 bytes per call site.
        log_message(const char *file, int line);

        // Used for MELON_LOG(severity) where severity != INFO.  Implied
        // are: ctr = 0, send_method = &log_message::send_to_log
        //
        // Using this constructor instead of the more complex constructor above
        // saves 17 bytes per call site.
        log_message(const char *file, int line, log_severity severity);

        // Constructor to log this message to a specified sink (if not NULL).
        // Implied are: ctr = 0, send_method = &log_message::send_to_sink_and_log if
        // also_send_to_log is true, send_method = &log_message::send_to_sink otherwise.
        log_message(const char *file, int line, log_severity severity, log_sink *sink,
                    bool also_send_to_log);

        // Constructor where we also give a vector<string> pointer
        // for storing the messages (if the pointer is not NULL).
        // Implied are: ctr = 0, send_method = &log_message::save_or_send_to_log.
        log_message(const char *file, int line, log_severity severity,
                    std::vector<std::string> *outvec);

        // Constructor where we also give a string pointer for storing the
        // message (if the pointer is not NULL).  Implied are: ctr = 0,
        // send_method = &log_message::write_to_string_and_log.
        log_message(const char *file, int line, log_severity severity,
                    std::string *message);

        // A special constructor used for check failures
        log_message(const char *file, int line, const CheckOpString &result);

        ~log_message();

        // Flush a buffered message to the sink set in the constructor.  Always
        // called by the destructor, it may also be called from elsewhere if
        // needed.  Only the first call is actioned; any later ones are ignored.
        void flush();

        // An arbitrary limit on the length of a single log message.  This
        // is so that streaming can be done more efficiently.
        static const size_t kMaxLogMessageLen;

        // Theses should not be called directly outside of logging.*,
        // only passed as SendMethod arguments to other log_message methods:
        void send_to_log();  // Actually dispatch to the logs
        void send_to_syslog_and_log();  // Actually dispatch to syslog and the logs

        // Call abort() or similar to perform MELON_LOG(FATAL) crash.
        static void fail();

        std::ostream &stream();

        int preserved_errno() const;

        // Must be called without the log_mutex held.  (L < log_mutex)
        static int64_t num_messages(int severity);

        struct log_message_data;

    private:

        // Fully internal SendMethod cases:
        void send_to_sink_and_log();  // Send to sink if provided and dispatch to the logs
        void send_to_sink();  // Send to sink if provided, do nothing otherwise.

        // Write to string if provided and dispatch to the logs.
        void write_to_string_and_log();

        void save_or_send_to_log();  // Save to stringvec if provided, else to logs

        void init(const char *file, int line, log_severity severity,
                  void (log_message::*send_method)());

        // Used to fill in crash information during MELON_LOG(FATAL) failures.
        void record_crash_reason(log_internal::crash_reason *reason);

        // Counts of messages sent at each priority:
        static int64_t num_messages_[NUM_SEVERITIES];  // under log_mutex

        // We keep the data in a separate struct so that each instance of
        // log_message uses less stack space.
        log_message_data *allocated_;
        log_message_data *data_;

        friend class log_destination;

        log_message(const log_message &);

        void operator=(const log_message &);
    };

    // This class happens to be thread-hostile because all instances share
    // a single data buffer, but since it can only be created just before
    // the process dies, we don't worry so much.
    class MELON_EXPORT log_message_fatal : public log_message {
    public:
        log_message_fatal(const char *file, int line);

        log_message_fatal(const char *file, int line, const CheckOpString &result);

        ~log_message_fatal();
    };

    // A non-macro interface to the log facility; (useful
    // when the logging level is not a compile-time constant).
    inline void LogAtLevel(int const severity, std::string const &msg) {
        log_message(__FILE__, __LINE__, severity).stream() << msg;
    }

// A macro alternative of LogAtLevel. New code may want to use this
// version since there are two advantages: 1. this version outputs the
// file name and the line number where this macro is put like other
// MELON_LOG macros, 2. this macro can be used as C++ stream.
#define MELON_LOG_AT_LEVEL(severity) melon::log::log_message(__FILE__, __LINE__, severity).stream()


    // Helper for MELON_CHECK_NOTNULL().
    //
    // In C++11, all cases can be handled by a single function. Since the value
    // category of the argument is preserved (also for rvalue references),
    // member initializer lists like the one below will compile correctly:
    //
    //   Foo()
    //     : x_(MELON_CHECK_NOTNULL(MethodReturningUniquePtr())) {}
    template<typename T>
    T check_not_null(const char *file, int line, const char *names, T &&t) {
        if (t == nullptr) {
            log_message_fatal(file, line, new std::string(names));
        }
        return std::forward<T>(t);
    }


    // Allow folks to put a counter in the LOG_EVERY_X()'ed messages. This
    // only works if ostream is a log_stream. If the ostream is not a
    // log_stream you'll get an assert saying as much at runtime.
    MELON_EXPORT std::ostream &operator<<(std::ostream &os,
                                          const PRIVATE_Counter &);


    // Derived class for MELON_PLOG*() above.
    class MELON_EXPORT errno_log_message : public log_message {
    public:

        errno_log_message(const char *file, int line, log_severity severity, uint64_t ctr,
                          void (log_message::*send_method)());

        // Postpends ": strerror(errno) [errno]".
        ~errno_log_message();

    private:

        errno_log_message(const errno_log_message &);

        void operator=(const errno_log_message &);
    };


    // This class is used to explicitly ignore values in the conditional
    // logging macros.  This avoids compiler warnings like "value computed
    // is not used" and "statement has no effect".

    class MELON_EXPORT log_message_voidify {
    public:
        log_message_voidify() {}

        // This has to be an operator with a precedence lower than << but
        // higher than ?:
        void operator&(std::ostream &) {}
    };


    // Flushes all log files that contains messages that are at least of
    // the specified severity level.  Thread-safe.
    MELON_EXPORT void flush_log_files(log_severity min_severity);

    // Flushes all log files that contains messages that are at least of
    // the specified severity level. Thread-hostile because it ignores
    // locking -- used for catastrophic failures.
    MELON_EXPORT void flush_log_files_unsafe(log_severity min_severity);

    //
    // Set the destination to which a particular severity level of log
    // messages is sent.  If base_filename is "", it means "don't log this
    // severity".  Thread-safe.
    //
    MELON_EXPORT void set_log_destination(log_severity severity,
                                          const char *base_filename);

    //
    // Set the basename of the symlink to the latest log file at a given
    // severity.  If symlink_basename is empty, do not make a symlink.  If
    // you don't call this function, the symlink basename is the
    // invocation name of the program.  Thread-safe.
    //
    MELON_EXPORT void set_log_symlink(log_severity severity,
                                      const char *symlink_basename);

    //
    // Used to send logs to some other kind of destination
    // Users should subclass log_sink and override send to do whatever they want.
    // Implementations must be thread-safe because a shared instance will
    // be called from whichever thread ran the MELON_LOG(XXX) line.
    class MELON_EXPORT log_sink {
    public:
        virtual ~log_sink();

        // Sink's logging logic (message_len is such as to exclude '\n' at the end).
        // This method can't use MELON_LOG() or MELON_CHECK() as logging system mutex(s) are held
        // during this call.
        virtual void send(log_severity severity, const char *full_filename,
                          const char *base_filename, int line,
                          const struct ::tm *tm_time,
                          const char *message, size_t message_len, int32_t /*usecs*/) {
            send(severity, full_filename, base_filename, line,
                 tm_time, message, message_len);
        }

        // This send() signature is obsolete.
        // New implementations should define this in terms of
        // the above send() method.
        virtual void send(log_severity severity, const char *full_filename,
                          const char *base_filename, int line,
                          const struct ::tm *tm_time,
                          const char *message, size_t message_len) = 0;

        // Redefine this to implement waiting for
        // the sink's logging logic to complete.
        // It will be called after each send() returns,
        // but before that log_message exits or crashes.
        // By default this function does nothing.
        // Using this function one can implement complex logic for send()
        // that itself involves logging; and do all this w/o causing deadlocks and
        // inconsistent rearrangement of log messages.
        // E.g. if a log_sink has thread-specific actions, the send() method
        // can simply add the message to a queue and wake up another thread that
        // handles real logging while itself making some MELON_LOG() calls;
        // WaitTillSent() can be implemented to wait for that logic to complete.
        // See our unittest for an example.
        virtual void WaitTillSent();

        // Returns the normal text output of the log message.
        // Can be useful to implement send().
        static std::string ToString(log_severity severity, const char *file, int line,
                                    const struct ::tm *tm_time,
                                    const char *message, size_t message_len,
                                    int32_t usecs);

        // Obsolete
        static std::string ToString(log_severity severity, const char *file, int line,
                                    const struct ::tm *tm_time,
                                    const char *message, size_t message_len) {
            return ToString(severity, file, line, tm_time, message, message_len, 0);
        }
    };

    // Add or remove a log_sink as a consumer of logging data.  Thread-safe.
    MELON_EXPORT void add_log_sink(log_sink *destination);

    MELON_EXPORT void remove_log_sink(log_sink *destination);

    //
    // Specify an "extension" added to the filename specified via
    // set_log_destination.  This applies to all severity levels.  It's
    // often used to append the port we're listening on to the logfile
    // name.  Thread-safe.
    //
    MELON_EXPORT void set_log_filename_extension(
            const char *filename_extension);

    //
    // Make it so that all log messages of at least a particular severity
    // are logged to stderr (in addition to logging to the usual log
    // file(s)).  Thread-safe.
    //
    MELON_EXPORT void set_stderr_logging(log_severity min_severity);

    //
    // Make it so that all log messages go only to stderr.  Thread-safe.
    //
    MELON_EXPORT void log_to_stderr();

    //
    // Make it so that all log messages of at least a particular severity are
    // logged via email to a list of addresses (in addition to logging to the
    // usual log file(s)).  The list of addresses is just a string containing
    // the email addresses to send to (separated by spaces, say).  Thread-safe.
    //
    MELON_EXPORT void set_email_logging(log_severity min_severity,
                                        const char *addresses);

    // A simple function that sends email. dest is a commma-separated
    // list of addressess.  Thread-safe.
    MELON_EXPORT bool SendEmail(const char *dest,
                                const char *subject, const char *body);

    MELON_EXPORT const std::vector<std::string> &GetLoggingDirectories();

    // For tests only:  Clear the internal [cached] list of logging directories to
    // force a refresh the next time GetLoggingDirectories is called.
    // Thread-hostile.
    void TestOnly_ClearLoggingDirectoriesList();

    // Returns a set of existing temporary directories, which will be a
    // subset of the directories returned by GetLogginDirectories().
    // Thread-safe.
    MELON_EXPORT void GetExistingTempDirectories(std::vector<std::string> *list);

    // Print any fatal message again -- useful to call from signal handler
    // so that the last thing in the output is the fatal message.
    // Thread-hostile, but a race is unlikely.
    MELON_EXPORT void reprint_fatal_message();

    // Truncate a log file that may be the append-only output of multiple
    // processes and hence can't simply be renamed/reopened (typically a
    // stdout/stderr).  If the file "path" is > "limit" bytes, copy the
    // last "keep" bytes to offset 0 and truncate the rest. Since we could
    // be racing with other writers, this approach has the potential to
    // lose very small amounts of data. For security, only follow symlinks
    // if the path is /proc/self/fd/*
    MELON_EXPORT void truncate_log_file(const char *path,
                                        int64_t limit, int64_t keep);

    // Truncate stdout and stderr if they are over the value specified by
    // --melon_max_log_size; keep the final 1MB.  This function has the same
    // race condition as truncate_log_file.
    MELON_EXPORT void truncate_stdout_stderr();

    // Return the string representation of the provided log_severity level.
    // Thread-safe.
    MELON_EXPORT const char *GetLogSeverityName(log_severity severity);

    // ---------------------------------------------------------------------
    // Implementation details that are not useful to most clients
    // ---------------------------------------------------------------------

    // A inner_logger is the interface used by logging modules to emit entries
    // to a log.  A typical implementation will dump formatted data to a
    // sequence of files.  We also provide interfaces that will forward
    // the data to another thread so that the invoker never blocks.
    // Implementations should be thread-safe since the logging system
    // will write to them from multiple threads.

    namespace base {

        class MELON_EXPORT inner_logger {
        public:
            virtual ~inner_logger();

            // Writes "message[0,message_len-1]" corresponding to an event that
            // occurred at "timestamp".  If "force_flush" is true, the log file
            // is flushed immediately.
            //
            // The input message has already been formatted as deemed
            // appropriate by the higher level logging facility.  For example,
            // textual log messages already contain timestamps, and the
            // file:linenumber header.
            virtual void write(bool force_flush,
                               time_t timestamp,
                               const char *message,
                               int message_len) = 0;

            // Flush any buffered messages
            virtual void flush() = 0;

            // Get the current MELON_LOG file size.
            // The returned value is approximate since some
            // logged data may not have been flushed to disk yet.
            virtual uint32_t log_size() = 0;
        };

        // Get the logger for the specified severity level.  The logger
        // remains the property of the logging module and should not be
        // deleted by the caller.  Thread-safe.
        extern MELON_EXPORT inner_logger *get_logger(log_severity level);

        // Set the logger for the specified severity level.  The logger
        // becomes the property of the logging module and should not
        // be deleted by the caller.  Thread-safe.
        extern MELON_EXPORT void set_logger(log_severity level, inner_logger *l);

    }

    // glibc has traditionally implemented two incompatible versions of
    // strerror_r(). There is a poorly defined convention for picking the
    // version that we want, but it is not clear whether it even works with
    // all versions of glibc.
    // So, instead, we provide this wrapper that automatically detects the
    // version that is in use, and then implements POSIX semantics.
    // N.B. In addition to what POSIX says, we also guarantee that "buf" will
    // be set to an empty string, if this function failed. This means, in most
    // cases, you do not need to check the error code and you can directly
    // use the value of "buf". It will never have an undefined value.
    // DEPRECATED: Use StrError(int) instead.
    MELON_EXPORT int posix_strerror_r(int err, char *buf, size_t len);

    // A thread-safe replacement for strerror(). Returns a string describing the
    // given POSIX error code.
    MELON_EXPORT std::string StrError(int err);

    // A class for which we define operator<<, which does nothing.
    class MELON_EXPORT null_stream : public log_message::log_stream {
    public:
        // Initialize the log_stream so the messages can be written somewhere
        // (they'll never be actually displayed). This will be needed if a
        // null_stream& is implicitly converted to log_stream&, in which case
        // the overloaded null_stream::operator<< will not be invoked.
        null_stream() : log_message::log_stream(message_buffer_, 1, 0) {}

        null_stream(const char * /*file*/, int /*line*/,
                    const CheckOpString & /*result*/) :
                log_message::log_stream(message_buffer_, 1, 0) {}

        null_stream &stream() { return *this; }

    private:
        // A very short buffer for messages (which we discard anyway). This
        // will be needed if null_stream& converted to log_stream& (e.g. as a
        // result of a conditional expression).
        char message_buffer_[2]{0, 0};
    };

    // Do nothing. This operator is inline, allowing the message to be
    // compiled away. The message will not be compiled away if we do
    // something like (flag ? MELON_LOG(INFO) : MELON_LOG(ERROR)) << message; when
    // SKIP_LOG=WARNING. In those cases, null_stream will be implicitly
    // converted to log_stream and the message will be computed and then
    // quietly discarded.
    template<class T>
    inline null_stream &operator<<(null_stream &str, const T &) { return str; }

    // Similar to null_stream, but aborts the program (without stack
    // trace), like log_message_fatal.
    class MELON_EXPORT null_stream_fatal : public null_stream {
    public:
        null_stream_fatal() {}

        null_stream_fatal(const char *file, int line, const CheckOpString &result) :
                null_stream(file, line, result) {}

        ~null_stream_fatal() throw() { _exit(1); }
    };

    // Install a signal handler that will dump signal information and a stack
    // trace when the program crashes on certain signals.  We'll install the
    // signal handler for the following signals.
    //
    // SIGSEGV, SIGILL, SIGFPE, SIGABRT, SIGBUS, and SIGTERM.
    //
    // By default, the signal handler will write the failure dump to the
    // standard error.  You can customize the destination by installing your
    // own writer function by InstallFailureWriter() below.
    //
    // Note on threading:
    //
    // The function should be called before threads are created, if you want
    // to use the failure signal handler for all threads.  The stack trace
    // will be shown only for the thread that receives the signal.  In other
    // words, stack traces of other threads won't be shown.
    MELON_EXPORT void InstallFailureSignalHandler();

    // Installs a function that is used for writing the failure dump.  "data"
    // is the pointer to the beginning of a message to be written, and "size"
    // is the size of the message.  You should not expect the data is
    // terminated with '\0'.
    MELON_EXPORT void InstallFailureWriter(
            void (*writer)(const char *data, int size));

}  // melon::log

# if MELON_DCHECK_IS_ON()
#  define MELON_DPLOG(...) MELON_PLOG(__VA_ARGS__)
#  define MELON_DPLOG_IF(...) MELON_PLOG_IF(__VA_ARGS__)
#  define MELON_DPCHECK(...) MELON_PCHECK(__VA_ARGS__)
#  define MELON_DVPLOG(...) MELON_VLOG(__VA_ARGS__)
# else
#  define MELON_DPLOG(...) MELON_DLOG(__VA_ARGS__)
#  define MELON_DPLOG_IF(...) MELON_DLOG_IF(__VA_ARGS__)
#  define MELON_DPCHECK(...) MELON_DCHECK(__VA_ARGS__)
#  define MELON_DVPLOG(...) MELON_DVLOG(__VA_ARGS__)
# endif


#define MELON_LOG_AT(severity, file, line)                                    \
    melon::log::log_message(file, line, melon::log::severity).stream()


#ifndef MELON_NOTIMPLEMENTED_POLICY
#if defined(MELON_PLATFORM_ANDROID) && defined(OFFICIAL_BUILD)
#define MELON_NOTIMPLEMENTED_POLICY 0
#else
// Select default policy: MELON_LOG(ERROR)
#define MELON_NOTIMPLEMENTED_POLICY 4
#endif
#endif

#if defined(MELON_COMPILER_GNUC) || defined(MELON_COMPILER_CLANG)
// On Linux, with GCC, we can use __PRETTY_FUNCTION__ to get the demangled name
// of the current function in the MELON_NOTIMPLEMENTED message.
#define MELON_NOTIMPLEMENTED_MSG "Not implemented reached in " << __PRETTY_FUNCTION__
#else
#define MELON_NOTIMPLEMENTED_MSG "NOT IMPLEMENTED"
#endif

#if MELON_NOTIMPLEMENTED_POLICY == 0
#define MELON_NOTIMPLEMENTED() MELON_EAT_STREAM_PARAMS
#elif MELON_NOTIMPLEMENTED_POLICY == 1
// TODO, figure out how to generate a warning
#define MELON_NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif MELON_NOTIMPLEMENTED_POLICY == 2
#define MELON_NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif MELON_NOTIMPLEMENTED_POLICY == 3
#define MELON_NOTIMPLEMENTED() MELON_NOTREACHED()
#elif MELON_NOTIMPLEMENTED_POLICY == 4
#define MELON_NOTIMPLEMENTED() MELON_LOG(ERROR) << MELON_NOTIMPLEMENTED_MSG
#elif MELON_NOTIMPLEMENTED_POLICY == 5
#define MELON_NOTIMPLEMENTED() do {                                   \
        static bool logged_once = false;                        \
        MELON_LOG_IF(ERROR, !logged_once) << MELON_NOTIMPLEMENTED_MSG;      \
        logged_once = true;                                     \
    } while(0);                                                 \
    MELON_EAT_STREAM_PARAMS
#endif

#if defined(NDEBUG) && defined(OS_CHROMEOS)
#define MELON_NOTREACHED() MELON_LOG(ERROR) << "MELON_NOTREACHED() hit in "       \
    << __FUNCTION__ << ". "
#else
#define MELON_NOTREACHED() MELON_DCHECK(false)
#endif

// Helper macro included by all *_EVERY_N macros.
#define MELON_LOG_IF_EVERY_N_IMPL(logifmacro, severity, condition, N)   \
    static std::atomic<int32_t> MELON_CONCAT(logeveryn_, __LINE__){-1}; \
    const static int MELON_CONCAT(logeveryn_sc_, __LINE__) = (N);       \
    const int MELON_CONCAT(logeveryn_c_, __LINE__) =                    \
        MELON_CONCAT(logeveryn_, __LINE__).fetch_add( 1) + 1; \
    logifmacro(severity, (condition) && MELON_CONCAT(logeveryn_c_, __LINE__) / \
               MELON_CONCAT(logeveryn_sc_, __LINE__) * MELON_CONCAT(logeveryn_sc_, __LINE__) \
               == MELON_CONCAT(logeveryn_c_, __LINE__))

// Helper macro included by all *_FIRST_N macros.
#define MELON_LOG_IF_FIRST_N_IMPL(logifmacro, severity, condition, N)   \
    static std::atomic<int32_t> MELON_CONCAT(logfstn_, __LINE__){0}; \
    logifmacro(severity, (condition) && MELON_CONCAT(logfstn_, __LINE__) < N && \
               MELON_CONCAT(logfstn_, __LINE__).fetch_add( 1) + 1 <= N)

// Helper macro included by all *_EVERY_SECOND macros.
#define MELON_LOG_IF_EVERY_SECOND_IMPL(logifmacro, severity, condition) \
    static ::melon::every_duration MELON_CONCAT(logeverys_, __LINE__)(melon::duration::seconds(1)); \
    logifmacro(severity, (condition) && MELON_CONCAT(logeverys_, __LINE__))

// ===============================================================

// Print a log for at most once.
// Almost zero overhead when the log was printed.
#ifndef MELON_LOG_ONCE
# define MELON_LOG_ONCE(severity) MELON_LOG_FIRST_N(severity, 1)
# define MELON_LOG_IF_ONCE(severity, condition) LOG_IF_FIRST_N(severity, condition, 1)
#endif

// Print a log after every N calls. First call always prints.
// Each call to this macro has a cost of relaxed atomic increment.
#ifndef MELON_LOG_EVERY_N
# define MELON_LOG_EVERY_N(severity, N)                                \
     MELON_LOG_IF_EVERY_N_IMPL(MELON_LOG_IF, severity, true, N)
# define MELON_LOG_IF_EVERY_N(severity, condition, N)                  \
     MELON_LOG_IF_EVERY_N_IMPL(MELON_LOG_IF, severity, condition, N)
#endif

// Print logs for first N calls.
// Almost zero overhead when the log was printed for N times
#ifndef MELON_LOG_FIRST_N
# define MELON_LOG_FIRST_N(severity, N)                                \
     MELON_LOG_IF_FIRST_N_IMPL(MELON_LOG_IF, severity, true, N)
# define LOG_IF_FIRST_N(severity, condition, N)                  \
     MELON_LOG_IF_FIRST_N_IMPL(MELON_LOG_IF, severity, condition, N)
#endif

// Print a log every second. First call always prints.
// Each call to this macro has a cost of calling gettimeofday.
#ifndef MELON_LOG_EVERY_SECOND
# define MELON_LOG_EVERY_SECOND(severity)                                \
     MELON_LOG_IF_EVERY_SECOND_IMPL(MELON_LOG_IF, severity, true)
# define LOG_IF_EVERY_SECOND(severity, condition)                \
     MELON_LOG_IF_EVERY_SECOND_IMPL(MELON_LOG_IF, severity, condition)
#endif

#ifndef MELON_PLOG_EVERY_N
# define MELON_PLOG_EVERY_N(severity, N)                               \
     MELON_LOG_IF_EVERY_N_IMPL(MELON_PLOG_IF, severity, true, N)
# define PLOG_IF_EVERY_N(severity, condition, N)                 \
     MELON_LOG_IF_EVERY_N_IMPL(MELON_PLOG_IF, severity, condition, N)
#endif

#ifndef MELON_PLOG_FIRST_N
# define MELON_PLOG_FIRST_N(severity, N)                               \
     MELON_LOG_IF_FIRST_N_IMPL(MELON_PLOG_IF, severity, true, N)
# define MELON_PLOG_IF_FIRST_N(severity, condition, N)                 \
     MELON_LOG_IF_FIRST_N_IMPL(MELON_PLOG_IF, severity, condition, N)
#endif

#ifndef MELON_PLOG_ONCE
# define MELON_PLOG_ONCE(severity) MELON_PLOG_FIRST_N(severity, 1)
# define MELON_PLOG_IF_ONCE(severity, condition) MELON_PLOG_IF_FIRST_N(severity, condition, 1)
#endif

#ifndef MELON_PLOG_EVERY_SECOND
# define MELON_PLOG_EVERY_SECOND(severity)                             \
     MELON_LOG_IF_EVERY_SECOND_IMPL(MELON_PLOG_IF, severity, true)
# define MELON_PLOG_IF_EVERY_SECOND(severity, condition)                       \
     MELON_LOG_IF_EVERY_SECOND_IMPL(MELON_PLOG_IF, severity, condition)
#endif

// DEBUG_MODE is for uses like
//   if (DEBUG_MODE) foo.CheckThatFoo();
// instead of
//   #ifndef NDEBUG
//     foo.CheckThatFoo();
//   #endif
//
// We tie its state to ENABLE_DLOG.
enum {
    DEBUG_MODE = MELON_DCHECK_IS_ON()
};

#endif  // MELON_LOG_LOGGING_H_
