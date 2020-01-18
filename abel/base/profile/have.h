//
// Created by liyinbin on 2019/12/11.
//

#ifndef ABEL_BASE_PROFILE_HAVE_H_
#define ABEL_BASE_PROFILE_HAVE_H_

#include <abel/base/profile/base.h>
#include <abel/base/profile/options.h>
// Included for the __GLIBC_PREREQ macro used below.
#include <limits.h>

// Included for the _STLPORT_VERSION macro used below.
#if defined(__cplusplus)
#include <cstddef>
#endif

#if defined(__APPLE__)
// Included for TARGET_OS_IPHONE, __IPHONE_OS_VERSION_MIN_REQUIRED,
// __IPHONE_8_0.
#include <Availability.h>
#include <TargetConditionals.h>
#endif


/* ABEL_HAVE_XXX_FEATURE */

#if !defined(ABEL_HAVE_EXTENSIONS_FEATURE) && !defined(ABEL_NO_HAVE_EXTENSIONS_FEATURE)
    #define ABEL_HAVE_EXTENSIONS_FEATURE 1
#endif


// ABEL_HAVE_TLS is defined to 1 when __thread should be supported.
// We assume __thread is supported on Linux when compiled with Clang or compiled
// against libstdc++ with _GLIBCXX_HAVE_TLS defined.
#ifdef ABEL_HAVE_TLS
#error ABEL_HAVE_TLS cannot be directly set
#elif defined(__linux__) && (defined(__clang__) || defined(_GLIBCXX_HAVE_TLS))
#define ABEL_HAVE_TLS 1
#endif

// ABEL_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE
//
// Checks whether `std::is_trivially_destructible<T>` is supported.
//
// Notes: All supported compilers using libc++ support this feature, as does
// gcc >= 4.8.1 using libstdc++, and Visual Studio.
#ifdef ABEL_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE
#error ABEL_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE cannot be directly set
#elif defined(_LIBCPP_VERSION) ||                                        \
    (!defined(__clang__) && defined(__GNUC__) && defined(__GLIBCXX__) && \
     (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))) ||        \
    defined(_MSC_VER)
#define ABEL_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE 1
#endif


// ABEL_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE
//
// Checks whether `std::is_trivially_default_constructible<T>` and
// `std::is_trivially_copy_constructible<T>` are supported.

// ABEL_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE
//
// Checks whether `std::is_trivially_copy_assignable<T>` is supported.

// Notes: Clang with libc++ supports these features, as does gcc >= 5.1 with
// either libc++ or libstdc++, and Visual Studio (but not NVCC).
#if defined(ABEL_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE)
#error ABEL_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE cannot be directly set
#elif defined(ABEL_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE)
#error ABEL_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE cannot directly set
#elif (defined(__clang__) && defined(_LIBCPP_VERSION)) ||        \
    (!defined(__clang__) && defined(__GNUC__) &&                 \
     (__GNUC__ > 7 || (__GNUC__ == 7 && __GNUC_MINOR__ >= 4)) && \
     (defined(_LIBCPP_VERSION) || defined(__GLIBCXX__))) ||      \
    (defined(_MSC_VER) && !defined(__NVCC__))
#define ABEL_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE 1
#define ABEL_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE 1
#endif


// ABEL_HAVE_SOURCE_LOCATION_CURRENT
//
// Indicates whether `abel::SourceLocation::current()` will return useful
// information in some contexts.
#ifndef ABEL_HAVE_SOURCE_LOCATION_CURRENT
#if ABEL_INTERNAL_HAS_KEYWORD(__builtin_LINE) && \
    ABEL_INTERNAL_HAS_KEYWORD(__builtin_FILE)
#define ABEL_HAVE_SOURCE_LOCATION_CURRENT 1
#endif
#endif

// ABEL_HAVE_THREAD_LOCAL
//
// Checks whether C++11's `thread_local` storage duration specifier is
// supported.
#ifdef ABEL_HAVE_THREAD_LOCAL
#error ABEL_HAVE_THREAD_LOCAL cannot be directly set
#elif defined(__APPLE__)
// Notes:
// * Xcode's clang did not support `thread_local` until version 8, and
//   even then not for all iOS < 9.0.
// * Xcode 9.3 started disallowing `thread_local` for 32-bit iOS simulator
//   targeting iOS 9.x.
// * Xcode 10 moves the deployment target check for iOS < 9.0 to link time
//   making __has_feature unreliable there.
//
// Otherwise, `__has_feature` is only supported by Clang so it has be inside
// `defined(__APPLE__)` check.
#if __has_feature(cxx_thread_local) && \
    !(TARGET_OS_IPHONE && __IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_9_0)
#define ABEL_HAVE_THREAD_LOCAL 1
#endif
#else  // !defined(__APPLE__)
#define ABEL_HAVE_THREAD_LOCAL 1
#endif

// There are platforms for which TLS should not be used even though the compiler
// makes it seem like it's supported (Android NDK < r12b for example).
// This is primarily because of linker problems and toolchain misconfiguration:
// abel does not intend to support this indefinitely. Currently, the newest
// toolchain that we intend to support that requires this behavior is the
// r11 NDK - allowing for a 5 year support window on that means this option
// is likely to be removed around June of 2021.
// TLS isn't supported until NDK r12b per
// https://developer.android.com/ndk/downloads/revision_history.html
// Since NDK r16, `__NDK_MAJOR__` and `__NDK_MINOR__` are defined in
// <android/ndk-version.h>. For NDK < r16, users should define these macros,
// e.g. `-D__NDK_MAJOR__=11 -D__NKD_MINOR__=0` for NDK r11.
#if defined(__ANDROID__) && defined(__clang__)
#if __has_include(<android/ndk-version.h>)
#include <android/ndk-version.h>
#endif  // __has_include(<android/ndk-version.h>)
#if defined(__ANDROID__) && defined(__clang__) && defined(__NDK_MAJOR__) && \
    defined(__NDK_MINOR__) &&                                               \
    ((__NDK_MAJOR__ < 12) || ((__NDK_MAJOR__ == 12) && (__NDK_MINOR__ < 1)))
#undef ABEL_HAVE_TLS
#undef ABEL_HAVE_THREAD_LOCAL
#endif
#endif  // defined(__ANDROID__) && defined(__clang__)

// Emscripten doesn't yet support `thread_local` or `__thread`.
// https://github.com/emscripten-core/emscripten/issues/3502
#if defined(__EMSCRIPTEN__)
#undef ABEL_HAVE_TLS
#undef ABEL_HAVE_THREAD_LOCAL
#endif  // defined(__EMSCRIPTEN__)

// ABEL_HAVE_INTRINSIC_INT128
//
// Checks whether the __int128 compiler extension for a 128-bit integral type is
// supported.
//
// Note: __SIZEOF_INT128__ is defined by Clang and GCC when __int128 is
// supported, but we avoid using it in certain cases:
// * On Clang:
//   * Building using Clang for Windows, where the Clang runtime library has
//     128-bit support only on LP64 architectures, but Windows is LLP64.
// * On Nvidia's nvcc:
//   * nvcc also defines __GNUC__ and __SIZEOF_INT128__, but not all versions
//     actually support __int128.
#ifdef ABEL_HAVE_INTRINSIC_INT128
#error ABEL_HAVE_INTRINSIC_INT128 cannot be directly set
#elif defined(__SIZEOF_INT128__)
#if (defined(__clang__) && !defined(_WIN32)) || \
    (defined(__CUDACC__) && __CUDACC_VER_MAJOR__ >= 9) ||                \
    (defined(__GNUC__) && !defined(__clang__) && !defined(__CUDACC__))
#define ABEL_HAVE_INTRINSIC_INT128 1
#elif defined(__CUDACC__)
// __CUDACC_VER__ is a full version number before CUDA 9, and is defined to a
// string explaining that it has been removed starting with CUDA 9. We use
// nested #ifs because there is no short-circuiting in the preprocessor.
// NOTE: `__CUDACC__` could be undefined while `__CUDACC_VER__` is defined.
#if __CUDACC_VER__ >= 70000
#define ABEL_HAVE_INTRINSIC_INT128 1
#endif  // __CUDACC_VER__ >= 70000
#endif  // defined(__CUDACC__)
#endif  // ABEL_HAVE_INTRINSIC_INT128

// ABEL_HAVE_EXCEPTIONS
//
// Checks whether the compiler both supports and enables exceptions. Many
// compilers support a "no exceptions" mode that disables exceptions.
//
// Generally, when ABEL_HAVE_EXCEPTIONS is not defined:
//
// * Code using `throw` and `try` may not compile.
// * The `noexcept` specifier will still compile and behave as normal.
// * The `noexcept` operator may still return `false`.
//
// For further details, consult the compiler's documentation.
#ifdef ABEL_HAVE_EXCEPTIONS
#error ABEL_HAVE_EXCEPTIONS cannot be directly set.

#elif defined(__clang__)
// TODO(calabrese)
// Switch to using __cpp_exceptions when we no longer support versions < 3.6.
// For details on this check, see:
//   http://releases.llvm.org/3.6.0/tools/clang/docs/ReleaseNotes.html#the-exceptions-macro
#if defined(__EXCEPTIONS) && __has_feature(cxx_exceptions)
#define ABEL_HAVE_EXCEPTIONS 1
#endif  // defined(__EXCEPTIONS) && __has_feature(cxx_exceptions)

// Handle remaining special cases and default to exceptions being supported.
#elif !(defined(__GNUC__) && (__GNUC__ < 5) && !defined(__EXCEPTIONS)) &&    \
    !(defined(__GNUC__) && (__GNUC__ >= 5) && !defined(__cpp_exceptions)) && \
    !(defined(_MSC_VER) && !defined(_CPPUNWIND))
#define ABEL_HAVE_EXCEPTIONS 1
#endif



// -----------------------------------------------------------------------------
// Platform Feature Checks
// -----------------------------------------------------------------------------

// Currently supported operating systems and associated preprocessor
// symbols:
//
//   Linux and Linux-derived           __linux__
//   Android                           __ANDROID__ (implies __linux__)
//   Linux (non-Android)               __linux__ && !__ANDROID__
//   Darwin (macOS and iOS)            __APPLE__
//   Akaros (http://akaros.org)        __ros__
//   Windows                           _WIN32
//   NaCL                              __native_client__
//   AsmJS                             __asmjs__
//   WebAssembly                       __wasm__
//   Fuchsia                           __Fuchsia__
//
// Note that since Android defines both __ANDROID__ and __linux__, one
// may probe for either Linux or Android by simply testing for __linux__.

// ABEL_HAVE_MMAP
//
// Checks whether the platform has an mmap(2) implementation as defined in
// POSIX.1-2001.
#ifdef ABEL_HAVE_MMAP
#error ABEL_HAVE_MMAP cannot be directly set
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) ||   \
    defined(__ros__) || defined(__native_client__) || defined(__asmjs__) || \
    defined(__wasm__) || defined(__Fuchsia__) || defined(__sun) || \
    defined(__ASYLO__)
#define ABEL_HAVE_MMAP 1
#endif

// ABEL_HAVE_PTHREAD_GETSCHEDPARAM
//
// Checks whether the platform implements the pthread_(get|set)schedparam(3)
// functions as defined in POSIX.1-2001.
#ifdef ABEL_HAVE_PTHREAD_GETSCHEDPARAM
#error ABEL_HAVE_PTHREAD_GETSCHEDPARAM cannot be directly set
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || \
    defined(__ros__)
#define ABEL_HAVE_PTHREAD_GETSCHEDPARAM 1
#endif

// ABEL_HAVE_SCHED_YIELD
//
// Checks whether the platform implements sched_yield(2) as defined in
// POSIX.1-2001.
#ifdef ABEL_HAVE_SCHED_YIELD
#error ABEL_HAVE_SCHED_YIELD cannot be directly set
#elif defined(__linux__) || defined(__ros__) || defined(__native_client__)
#define ABEL_HAVE_SCHED_YIELD 1
#endif

// ABEL_HAVE_ALARM
//
// Checks whether the platform supports the <signal.h> header and alarm(2)
// function as standardized in POSIX.1-2001.
#ifdef ABEL_HAVE_ALARM
#error ABEL_HAVE_ALARM cannot be directly set
#elif defined(__GOOGLE_GRTE_VERSION__)
// feature tests for Google's GRTE
#define ABEL_HAVE_ALARM 1
#elif defined(__GLIBC__)
// feature test for glibc
#define ABEL_HAVE_ALARM 1
#elif defined(_MSC_VER)
// feature tests for Microsoft's library
#elif defined(__MINGW32__)
// mingw32 doesn't provide alarm(2):
// https://osdn.net/projects/mingw/scm/git/mingw-org-wsl/blobs/5.2-trunk/mingwrt/include/unistd.h
// mingw-w64 provides a no-op implementation:
// https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-crt/misc/alarm.c
#elif defined(__EMSCRIPTEN__)
// emscripten doesn't support signals
#elif defined(__Fuchsia__)
// Signals don't exist on fuchsia.
#elif defined(__native_client__)
#else
// other standard libraries
#define ABEL_HAVE_ALARM 1
#endif



// macOS 10.13 and iOS 10.11 don't let you use <any>, <optional>, or <variant>
// even though the headers exist and are publicly noted to work.  See
// libc++ spells out the availability requirements in the file
// llvm-project/libcxx/include/__config via the #define
// _LIBCPP_AVAILABILITY_BAD_OPTIONAL_ACCESS.
#if defined(__APPLE__) && defined(_LIBCPP_VERSION) && \
  ((defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && \
   __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 101400) || \
  (defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) && \
   __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ < 120000) || \
  (defined(__ENVIRONMENT_WATCH_OS_VERSION_MIN_REQUIRED__) && \
   __ENVIRONMENT_WATCH_OS_VERSION_MIN_REQUIRED__ < 120000) || \
  (defined(__ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__) && \
   __ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__ < 50000))
#define ABEL_INTERNAL_APPLE_CXX17_TYPES_UNAVAILABLE 1
#else
#define ABEL_INTERNAL_APPLE_CXX17_TYPES_UNAVAILABLE 0
#endif


// ABEL_HAVE_STD_ANY
//
// Checks whether C++17 std::any is available by checking whether <any> exists.
#ifdef ABEL_HAVE_STD_ANY
#error "ABEL_HAVE_STD_ANY cannot be directly set."
#endif

#ifdef __has_include
#if __has_include(<any>) && __cplusplus >= 201703L && \
    !ABEL_INTERNAL_APPLE_CXX17_TYPES_UNAVAILABLE
#define ABEL_HAVE_STD_ANY 1
#endif
#endif

// ABEL_USES_STD_ANY
//
// Indicates whether abel::any is an alias for std::any.
#if !defined(ABEL_OPTION_USE_STD_ANY)
#error options.h is misconfigured.
#elif ABEL_OPTION_USE_STD_ANY == 0 || \
    (ABEL_OPTION_USE_STD_ANY == 2 && !defined(ABEL_HAVE_STD_ANY))
#undef ABEL_USES_STD_ANY
#elif ABEL_OPTION_USE_STD_ANY == 1 || \
    (ABEL_OPTION_USE_STD_ANY == 2 && defined(ABEL_HAVE_STD_ANY))
#define ABEL_USES_STD_ANY 1
#else
#error options.h is misconfigured.
#endif

// ABEL_HAVE_STD_OPTIONAL
//
// Checks whether C++17 std::optional is available.
#ifdef ABEL_HAVE_STD_OPTIONAL
#error "ABEL_HAVE_STD_OPTIONAL cannot be directly set."
#endif

#ifdef __has_include
#if __has_include(<optional>) && __cplusplus >= 201703L && \
    !ABEL_INTERNAL_APPLE_CXX17_TYPES_UNAVAILABLE
#define ABEL_HAVE_STD_OPTIONAL 1
#endif
#endif

// ABEL_HAVE_STD_VARIANT
//
// Checks whether C++17 std::variant is available.
#ifdef ABEL_HAVE_STD_VARIANT
#error "ABEL_HAVE_STD_VARIANT cannot be directly set."
#endif

#ifdef __has_include
#if __has_include(<variant>) && __cplusplus >= 201703L && \
    !ABEL_INTERNAL_APPLE_CXX17_TYPES_UNAVAILABLE
#define ABEL_HAVE_STD_VARIANT 1
#endif
#endif

// ABEL_HAVE_STD_STRING_VIEW
//
// Checks whether C++17 std::string_view is available.
#ifdef ABEL_HAVE_STD_STRING_VIEW
#error "ABEL_HAVE_STD_STRING_VIEW cannot be directly set."
#endif


#ifdef __has_include
#if __has_include(<string_view>) && __cplusplus >= 201703L
#define ABEL_HAVE_STD_STRING_VIEW 1
#endif
#endif

// For MSVC, `__has_include` is supported in VS 2017 15.3, which is later than
// the support for <optional>, <any>, <string_view>, <variant>. So we use
// _MSC_VER to check whether we have VS 2017 RTM (when <optional>, <any>,
// <string_view>, <variant> is implemented) or higher. Also, `__cplusplus` is
// not correctly set by MSVC, so we use `_MSVC_LANG` to check the language
// version.
// TODO(zhangxy): fix tests before enabling aliasing for `std::any`.
#if defined(_MSC_VER) && _MSC_VER >= 1910 && \
    ((defined(_MSVC_LANG) && _MSVC_LANG > 201402) || __cplusplus > 201402)
// #define ABEL_HAVE_STD_ANY 1
#define ABEL_HAVE_STD_OPTIONAL 1
#define ABEL_HAVE_STD_VARIANT 1
#define ABEL_HAVE_STD_STRING_VIEW 1
#endif



// ABEL_USES_STD_OPTIONAL
//
// Indicates whether abel::optional is an alias for std::optional.
#if !defined(ABEL_OPTION_USE_STD_OPTIONAL)
#error options.h is misconfigured.
#elif ABEL_OPTION_USE_STD_OPTIONAL == 0 || \
    (ABEL_OPTION_USE_STD_OPTIONAL == 2 && !defined(ABEL_HAVE_STD_OPTIONAL))
#undef ABEL_USES_STD_OPTIONAL
#elif ABEL_OPTION_USE_STD_OPTIONAL == 1 || \
    (ABEL_OPTION_USE_STD_OPTIONAL == 2 && defined(ABEL_HAVE_STD_OPTIONAL))
#define ABEL_USES_STD_OPTIONAL 1
#else
#error options.h is misconfigured.
#endif

// ABEL_USES_STD_VARIANT
//
// Indicates whether abel::variant is an alias for std::variant.
#if !defined(ABEL_OPTION_USE_STD_VARIANT)
#error options.h is misconfigured.
#elif ABEL_OPTION_USE_STD_VARIANT == 0 || \
    (ABEL_OPTION_USE_STD_VARIANT == 2 && !defined(ABEL_HAVE_STD_VARIANT))
#undef ABEL_USES_STD_VARIANT
#elif ABEL_OPTION_USE_STD_VARIANT == 1 || \
    (ABEL_OPTION_USE_STD_VARIANT == 2 && defined(ABEL_HAVE_STD_VARIANT))
#define ABEL_USES_STD_VARIANT 1
#else
#error options.h is misconfigured.
#endif

// ABEL_USES_STD_STRING_VIEW
//
// Indicates whether abel::string_view is an alias for std::string_view.
#if !defined(ABEL_OPTION_USE_STD_STRING_VIEW)
#error options.h is misconfigured.
#elif ABEL_OPTION_USE_STD_STRING_VIEW == 0 || \
    (ABEL_OPTION_USE_STD_STRING_VIEW == 2 &&  \
     !defined(ABEL_HAVE_STD_STRING_VIEW))
#undef ABEL_USES_STD_STRING_VIEW
#elif ABEL_OPTION_USE_STD_STRING_VIEW == 1 || \
    (ABEL_OPTION_USE_STD_STRING_VIEW == 2 &&  \
     defined(ABEL_HAVE_STD_STRING_VIEW))
#define ABEL_USES_STD_STRING_VIEW 1
#else
#error options.h is misconfigured.
#endif

// In debug mode, MSVC 2017's std::variant throws a EXCEPTION_ACCESS_VIOLATION
// SEH exception from emplace for variant<SomeStruct> when constructing the
// struct can throw. This defeats some of variant_test and
// variant_exception_safety_test.
#if defined(_MSC_VER) && _MSC_VER >= 1700 && defined(_DEBUG)
#define ABEL_INTERNAL_MSVC_2017_DBG_MODE
#endif

#undef ABEL_INTERNAL_HAS_KEYWORD

/* ABEL_HAVE_XXX_LIBRARY */

// Dinkumware
#if !defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && !defined(ABEL_NO_HAVE_DINKUMWARE_CPP_LIBRARY)
    #if defined(__cplusplus)
ABEL_DISABLE_ALL_VC_WARNINGS()
        #include <cstddef> // Need to trigger the compilation of yvals.h without directly using <yvals.h> because it might not exist.
        ABEL_RESTORE_ALL_VC_WARNINGS()
    #endif

    #if defined(__cplusplus) && defined(_CPPLIB_VER) /* If using the Dinkumware Standard library... */
        #define ABEL_HAVE_DINKUMWARE_CPP_LIBRARY 1
    #else
        #define ABEL_NO_HAVE_DINKUMWARE_CPP_LIBRARY 1
    #endif
#endif

// GCC libstdc++
#if !defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && !defined(ABEL_NO_HAVE_LIBSTDCPP_LIBRARY)
    #if defined(__GLIBCXX__) /* If using libstdc++ ... */
        #define ABEL_HAVE_LIBSTDCPP_LIBRARY 1
    #else
        #define ABEL_NO_HAVE_LIBSTDCPP_LIBRARY 1
    #endif
#endif

// Clang libc++
#if !defined(ABEL_HAVE_LIBCPP_LIBRARY) && !defined(ABEL_NO_HAVE_LIBCPP_LIBRARY)
    #if defined(__clang__)
#if defined(__cplusplus) && __has_include(<__config>)
    #define ABEL_HAVE_LIBCPP_LIBRARY 1 // We could also #include <ciso646> and check if defined(_LIBCPP_VERSION).
#endif
    #endif

    #if !defined(ABEL_HAVE_LIBCPP_LIBRARY)
        #define ABEL_NO_HAVE_LIBCPP_LIBRARY 1
    #endif
#endif


/* ABEL_HAVE_XXX_H */

// #include <sys/types.h>
#if !defined(ABEL_HAVE_SYS_TYPES_H) && !defined(ABEL_NO_HAVE_SYS_TYPES_H)
    #define ABEL_HAVE_SYS_TYPES_H 1
#endif

// #include <io.h> (and not sys/io.h or asm/io.h)
#if !defined(ABEL_HAVE_IO_H) && !defined(ABEL_NO_HAVE_IO_H)
// Unix doesn't have Microsoft's <io.h> but has the same functionality in <fcntl.h> and <sys/stat.h>.
    #if defined(ABEL_PLATFORM_MICROSOFT)
        #define ABEL_HAVE_IO_H 1
    #else
        #define ABEL_NO_HAVE_IO_H 1
    #endif
#endif

// #include <inttypes.h>
#if !defined(ABEL_HAVE_INTTYPES_H) && !defined(ABEL_NO_HAVE_INTTYPES_H)
    #if !defined(ABEL_PLATFORM_MICROSOFT)
        #define ABEL_HAVE_INTTYPES_H 1
    #else
        #define ABEL_NO_HAVE_INTTYPES_H 1
    #endif
#endif

// #include <unistd.h>
#if !defined(ABEL_HAVE_UNISTD_H) && !defined(ABEL_NO_HAVE_UNISTD_H)
    #if defined(ABEL_PLATFORM_UNIX)
        #define ABEL_HAVE_UNISTD_H 1
    #else
        #define ABEL_NO_HAVE_UNISTD_H 1
    #endif
#endif

// #include <sys/time.h>
#if !defined(ABEL_HAVE_SYS_TIME_H) && !defined(ABEL_NO_HAVE_SYS_TIME_H)
    #if !defined(ABEL_PLATFORM_MICROSOFT) && !defined(_CPPLIB_VER) /* _CPPLIB_VER indicates Dinkumware. */
        #define ABEL_HAVE_SYS_TIME_H 1 /* defines struct timeval */
    #else
        #define ABEL_NO_HAVE_SYS_TIME_H 1
    #endif
#endif

// #include <ptrace.h>
#if !defined(ABEL_HAVE_SYS_PTRACE_H) && !defined(ABEL_NO_HAVE_SYS_PTRACE_H)
    #if defined(ABEL_PLATFORM_UNIX) && !defined(__CYGWIN__) && (defined(ABEL_PLATFORM_DESKTOP) || defined(ABEL_PLATFORM_SERVER))
        #define ABEL_HAVE_SYS_PTRACE_H 1 /* declares the ptrace function */
    #else
        #define ABEL_NO_HAVE_SYS_PTRACE_H 1
    #endif
#endif

// #include <sys/stat.h>
#if !defined(ABEL_HAVE_SYS_STAT_H) && !defined(ABEL_NO_HAVE_SYS_STAT_H)
    #if (defined(ABEL_PLATFORM_UNIX) && !(defined(ABEL_PLATFORM_SONY) && defined(ABEL_PLATFORM_CONSOLE))) || defined(__APPLE__) || defined(ABEL_PLATFORM_ANDROID)
        #define ABEL_HAVE_SYS_STAT_H 1 /* declares the stat struct and function */
    #else
        #define ABEL_NO_HAVE_SYS_STAT_H 1
    #endif
#endif

// #include <locale.h>
#if !defined(ABEL_HAVE_LOCALE_H) && !defined(ABEL_NO_HAVE_LOCALE_H)
    #define ABEL_HAVE_LOCALE_H 1
#endif

// #include <signal.h>
#if !defined(ABEL_HAVE_SIGNAL_H) && !defined(ABEL_NO_HAVE_SIGNAL_H)
    #if !defined(ABEL_PLATFORM_BSD) && !defined(ABEL_PLATFORM_SONY) && !defined(CS_UNDEFINED_STRING)
        #define ABEL_HAVE_SIGNAL_H 1
    #else
        #define ABEL_NO_HAVE_SIGNAL_H 1
    #endif
#endif

// #include <sys/signal.h>
#if !defined(ABEL_HAVE_SYS_SIGNAL_H) && !defined(ABEL_NO_HAVE_SYS_SIGNAL_H)
    #if defined(ABEL_PLATFORM_BSD) || defined(ABEL_PLATFORM_SONY)
        #define ABEL_HAVE_SYS_SIGNAL_H 1
    #else
        #define ABEL_NO_HAVE_SYS_SIGNAL_H 1
    #endif
#endif

// #include <pthread.h>
#if !defined(ABEL_HAVE_PTHREAD_H) && !defined(ABEL_NO_HAVE_PTHREAD_H)
    #if defined(ABEL_PLATFORM_UNIX) || defined(ABEL_PLATFORM_APPLE) || defined(ABEL_PLATFORM_POSIX)
        #define ABEL_HAVE_PTHREAD_H 1 /* It can be had under Microsoft/Windows with the http://sourceware.org/pthreads-win32/ library */
    #else
        #define ABEL_NO_HAVE_PTHREAD_H 1
    #endif
#endif

// #include <wchar.h>
#if !defined(ABEL_HAVE_WCHAR_H) && !defined(ABEL_NO_HAVE_WCHAR_H)
    #if defined(ABEL_PLATFORM_DESKTOP) && defined(ABEL_PLATFORM_UNIX) && defined(ABEL_PLATFORM_SONY) && defined(ABEL_PLATFORM_APPLE)
        #define ABEL_HAVE_WCHAR_H 1
    #else
        #define ABEL_NO_HAVE_WCHAR_H 1
    #endif
#endif

// #include <malloc.h>
#if !defined(ABEL_HAVE_MALLOC_H) && !defined(ABEL_NO_HAVE_MALLOC_H)
    #if defined(_MSC_VER) || defined(__MINGW32__)
        #define ABEL_HAVE_MALLOC_H 1
    #else
        #define ABEL_NO_HAVE_MALLOC_H 1
    #endif
#endif

// #include <alloca.h>
#if !defined(ABEL_HAVE_ALLOCA_H) && !defined(ABEL_NO_HAVE_ALLOCA_H)
    #if !defined(ABEL_HAVE_MALLOC_H) && !defined(ABEL_PLATFORM_SONY)
        #define ABEL_HAVE_ALLOCA_H 1
    #else
        #define ABEL_NO_HAVE_ALLOCA_H 1
    #endif
#endif

// #include <execinfo.h>
#if !defined(ABEL_HAVE_EXECINFO_H) && !defined(ABEL_NO_HAVE_EXECINFO_H)
    #if (defined(ABEL_PLATFORM_LINUX) || defined(ABEL_PLATFORM_OSX)) && !defined(ABEL_PLATFORM_ANDROID)
        #define ABEL_HAVE_EXECINFO_H 1
    #else
        #define ABEL_NO_HAVE_EXECINFO_H 1
    #endif
#endif

// #include <semaphore.h> (Unix semaphore support)
#if !defined(ABEL_HAVE_SEMAPHORE_H) && !defined(ABEL_NO_HAVE_SEMAPHORE_H)
    #if defined(ABEL_PLATFORM_UNIX)
        #define ABEL_HAVE_SEMAPHORE_H 1
    #else
        #define ABEL_NO_HAVE_SEMAPHORE_H 1
    #endif
#endif

// #include <dirent.h> (Unix semaphore support)
#if !defined(ABEL_HAVE_DIRENT_H) && !defined(ABEL_NO_HAVE_DIRENT_H)
    #if defined(ABEL_PLATFORM_UNIX) && !defined(ABEL_PLATFORM_CONSOLE)
        #define ABEL_HAVE_DIRENT_H 1
    #else
        #define ABEL_NO_HAVE_DIRENT_H 1
    #endif
#endif

// #include <array>, <forward_list>, <ununordered_set>, <unordered_map>
#if !defined(ABEL_HAVE_CPP11_CONTAINERS) && !defined(ABEL_NO_HAVE_CPP11_CONTAINERS)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) // Dinkumware. VS2010+
        #define ABEL_HAVE_CPP11_CONTAINERS 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4004) // Actually GCC 4.3 supports array and unordered_
        #define ABEL_HAVE_CPP11_CONTAINERS 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_CONTAINERS 1
    #else
        #define ABEL_NO_HAVE_CPP11_CONTAINERS 1
    #endif
#endif

// #include <atomic>
#if !defined(ABEL_HAVE_CPP11_ATOMIC) && !defined(ABEL_NO_HAVE_CPP11_ATOMIC)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
        #define ABEL_HAVE_CPP11_ATOMIC 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4007)
        #define ABEL_HAVE_CPP11_ATOMIC 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_ATOMIC 1
    #else
        #define ABEL_NO_HAVE_CPP11_ATOMIC 1
    #endif
#endif

// #include <condition_variable>
#if !defined(ABEL_HAVE_CPP11_CONDITION_VARIABLE) && !defined(ABEL_NO_HAVE_CPP11_CONDITION_VARIABLE)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
        #define ABEL_HAVE_CPP11_CONDITION_VARIABLE 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4007)
        #define ABEL_HAVE_CPP11_CONDITION_VARIABLE 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_CONDITION_VARIABLE 1
    #else
        #define ABEL_NO_HAVE_CPP11_CONDITION_VARIABLE 1
    #endif
#endif

// #include <mutex>
#if !defined(ABEL_HAVE_CPP11_MUTEX) && !defined(ABEL_NO_HAVE_CPP11_MUTEX)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
        #define ABEL_HAVE_CPP11_MUTEX 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4007)
        #define ABEL_HAVE_CPP11_MUTEX 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_MUTEX 1
    #else
        #define ABEL_NO_HAVE_CPP11_MUTEX 1
    #endif
#endif

// #include <thread>
#if !defined(ABEL_HAVE_CPP11_THREAD) && !defined(ABEL_NO_HAVE_CPP11_THREAD)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
        #define ABEL_HAVE_CPP11_THREAD 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4007)
        #define ABEL_HAVE_CPP11_THREAD 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_THREAD 1
    #else
        #define ABEL_NO_HAVE_CPP11_THREAD 1
    #endif
#endif

// #include <future>
#if !defined(ABEL_HAVE_CPP11_FUTURE) && !defined(ABEL_NO_HAVE_CPP11_FUTURE)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
        #define ABEL_HAVE_CPP11_FUTURE 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4005)
        #define ABEL_HAVE_CPP11_FUTURE 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_FUTURE 1
    #else
        #define ABEL_NO_HAVE_CPP11_FUTURE 1
    #endif
#endif


// #include <type_traits>
#if !defined(ABEL_HAVE_CPP11_TYPE_TRAITS) && !defined(ABEL_NO_HAVE_CPP11_TYPE_TRAITS)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
        #define ABEL_HAVE_CPP11_TYPE_TRAITS 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4007) // Prior versions of libstdc++ have incomplete support for C++11 type traits.
        #define ABEL_HAVE_CPP11_TYPE_TRAITS 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_TYPE_TRAITS 1
    #else
        #define ABEL_NO_HAVE_CPP11_TYPE_TRAITS 1
    #endif
#endif

// #include <tuple>
#if !defined(ABEL_HAVE_CPP11_TUPLES) && !defined(ABEL_NO_HAVE_CPP11_TUPLES)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) // Dinkumware. VS2010+
        #define ABEL_HAVE_CPP11_TUPLES 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4003)
        #define ABEL_HAVE_CPP11_TUPLES 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_TUPLES 1
    #else
        #define ABEL_NO_HAVE_CPP11_TUPLES 1
    #endif
#endif

// #include <regex>
#if !defined(ABEL_HAVE_CPP11_REGEX) && !defined(ABEL_NO_HAVE_CPP11_REGEX)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) && (defined(_HAS_EXCEPTIONS) && _HAS_EXCEPTIONS) // Dinkumware. VS2012+
        #define ABEL_HAVE_CPP11_REGEX 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4003)
        #define ABEL_HAVE_CPP11_REGEX 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_REGEX 1
    #else
        #define ABEL_NO_HAVE_CPP11_REGEX 1
    #endif
#endif

// #include <random>
#if !defined(ABEL_HAVE_CPP11_RANDOM) && !defined(ABEL_NO_HAVE_CPP11_RANDOM)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) // Dinkumware. VS2010+
        #define ABEL_HAVE_CPP11_RANDOM 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4005)
        #define ABEL_HAVE_CPP11_RANDOM 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_RANDOM 1
    #else
        #define ABEL_NO_HAVE_CPP11_RANDOM 1
    #endif
#endif

// #include <chrono>
#if !defined(ABEL_HAVE_CPP11_CHRONO) && !defined(ABEL_NO_HAVE_CPP11_CHRONO)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
        #define ABEL_HAVE_CPP11_CHRONO 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4007) // chrono was broken in glibc prior to 4.7.
        #define ABEL_HAVE_CPP11_CHRONO 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_CHRONO 1
    #else
        #define ABEL_NO_HAVE_CPP11_CHRONO 1
    #endif
#endif

// #include <scoped_allocator>
#if !defined(ABEL_HAVE_CPP11_SCOPED_ALLOCATOR) && !defined(ABEL_NO_HAVE_CPP11_SCOPED_ALLOCATOR)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 540) // Dinkumware. VS2012+
        #define ABEL_HAVE_CPP11_SCOPED_ALLOCATOR 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4007)
        #define ABEL_HAVE_CPP11_SCOPED_ALLOCATOR 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_SCOPED_ALLOCATOR 1
    #else
        #define ABEL_NO_HAVE_CPP11_SCOPED_ALLOCATOR 1
    #endif
#endif

// #include <initializer_list>
#if !defined(ABEL_HAVE_CPP11_INITIALIZER_LIST) && !defined(ABEL_NO_HAVE_CPP11_INITIALIZER_LIST)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) && !defined(ABEL_COMPILER_NO_INITIALIZER_LISTS) // Dinkumware. VS2010+
        #define ABEL_HAVE_CPP11_INITIALIZER_LIST 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_CLANG) && (ABEL_COMPILER_VERSION >= 301) && !defined(ABEL_COMPILER_NO_INITIALIZER_LISTS) && !defined(ABEL_PLATFORM_APPLE)
        #define ABEL_HAVE_CPP11_INITIALIZER_LIST 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBCPP_LIBRARY) && defined(ABEL_COMPILER_CLANG) && (ABEL_COMPILER_VERSION >= 301) && !defined(ABEL_COMPILER_NO_INITIALIZER_LISTS) && !defined(ABEL_PLATFORM_APPLE)
        #define ABEL_HAVE_CPP11_INITIALIZER_LIST 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4004) && !defined(ABEL_COMPILER_NO_INITIALIZER_LISTS) && !defined(ABEL_PLATFORM_APPLE)
        #define ABEL_HAVE_CPP11_INITIALIZER_LIST 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1) && !defined(ABEL_COMPILER_NO_INITIALIZER_LISTS)
        #define ABEL_HAVE_CPP11_INITIALIZER_LIST 1
    #else
        #define ABEL_NO_HAVE_CPP11_INITIALIZER_LIST 1
    #endif
#endif

// #include <system_error>
#if !defined(ABEL_HAVE_CPP11_SYSTEM_ERROR) && !defined(ABEL_NO_HAVE_CPP11_SYSTEM_ERROR)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) && !(defined(_HAS_CPP0X) && _HAS_CPP0X) // Dinkumware. VS2010+
        #define ABEL_HAVE_CPP11_SYSTEM_ERROR 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_CLANG) && (ABEL_COMPILER_VERSION >= 301) && !defined(ABEL_PLATFORM_APPLE)
        #define ABEL_HAVE_CPP11_SYSTEM_ERROR 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4004) && !defined(ABEL_PLATFORM_APPLE)
        #define ABEL_HAVE_CPP11_SYSTEM_ERROR 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_SYSTEM_ERROR 1
    #else
        #define ABEL_NO_HAVE_CPP11_SYSTEM_ERROR 1
    #endif
#endif

// #include <codecvt>
#if !defined(ABEL_HAVE_CPP11_CODECVT) && !defined(ABEL_NO_HAVE_CPP11_CODECVT)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) // Dinkumware. VS2010+
        #define ABEL_HAVE_CPP11_CODECVT 1
// Future versions of libc++ may support this header.  However, at the moment there isn't
// a reliable way of detecting if this header is available.
//#elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4008)
//    #define ABEL_HAVE_CPP11_CODECVT 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_CODECVT 1
    #else
        #define ABEL_NO_HAVE_CPP11_CODECVT 1
    #endif
#endif

// #include <typeindex>
#if !defined(ABEL_HAVE_CPP11_TYPEINDEX) && !defined(ABEL_NO_HAVE_CPP11_TYPEINDEX)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) // Dinkumware. VS2010+
        #define ABEL_HAVE_CPP11_TYPEINDEX 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4006)
        #define ABEL_HAVE_CPP11_TYPEINDEX 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_TYPEINDEX 1
    #else
        #define ABEL_NO_HAVE_CPP11_TYPEINDEX 1
    #endif
#endif




/* ABEL_HAVE_XXX_DECL */

#if !defined(ABEL_HAVE_mkstemps_DECL) && !defined(ABEL_NO_HAVE_mkstemps_DECL)
    #if defined(ABEL_PLATFORM_APPLE) || defined(CS_UNDEFINED_STRING)
        #define ABEL_HAVE_mkstemps_DECL 1
    #else
        #define ABEL_NO_HAVE_mkstemps_DECL 1
    #endif
#endif

#if !defined(ABEL_HAVE_gettimeofday_DECL) && !defined(ABEL_NO_HAVE_gettimeofday_DECL)
    #if defined(ABEL_PLATFORM_POSIX) /* Posix means Linux, Unix, and Macintosh OSX, among others (including Linux-based mobile platforms). */
        #define ABEL_HAVE_gettimeofday_DECL 1
    #else
        #define ABEL_NO_HAVE_gettimeofday_DECL 1
    #endif
#endif

#if !defined(ABEL_HAVE_strcasecmp_DECL) && !defined(ABEL_NO_HAVE_strcasecmp_DECL)
    #if !defined(ABEL_PLATFORM_MICROSOFT)
        #define ABEL_HAVE_strcasecmp_DECL  1     /* This is found as stricmp when not found as strcasecmp */
        #define ABEL_HAVE_strncasecmp_DECL 1
    #else
        #define ABEL_HAVE_stricmp_DECL  1
        #define ABEL_HAVE_strnicmp_DECL 1
    #endif
#endif

#if !defined(ABEL_HAVE_mmap_DECL) && !defined(ABEL_NO_HAVE_mmap_DECL)
    #if defined(ABEL_PLATFORM_POSIX)
        #define ABEL_HAVE_mmap_DECL 1 /* mmap functionality varies significantly between systems. */
    #else
        #define ABEL_NO_HAVE_mmap_DECL 1
    #endif
#endif

#if !defined(ABEL_HAVE_fopen_DECL) && !defined(ABEL_NO_HAVE_fopen_DECL)
    #define ABEL_HAVE_fopen_DECL 1 /* C FILE functionality such as fopen */
#endif

#if !defined(ABEL_HAVE_ISNAN) && !defined(ABEL_NO_HAVE_ISNAN)
    #if defined(ABEL_PLATFORM_MICROSOFT) && !defined(ABEL_PLATFORM_MINGW)
        #define ABEL_HAVE_ISNAN(x)  _isnan(x)          /* declared in <math.h> */
        #define ABEL_HAVE_ISINF(x)  !_finite(x)
    #elif defined(ABEL_PLATFORM_APPLE)
        #define ABEL_HAVE_ISNAN(x)  std::isnan(x)      /* declared in <cmath> */
        #define ABEL_HAVE_ISINF(x)  std::isinf(x)
    #elif defined(ABEL_PLATFORM_ANDROID)
        #define ABEL_HAVE_ISNAN(x)  __builtin_isnan(x) /* There are a number of standard libraries for Android and it's hard to tell them apart, so just go with builtins */
        #define ABEL_HAVE_ISINF(x)  __builtin_isinf(x)
    #elif defined(__GNUC__) && defined(__CYGWIN__)
        #define ABEL_HAVE_ISNAN(x)  __isnand(x)        /* declared nowhere, it seems. */
        #define ABEL_HAVE_ISINF(x)  __isinfd(x)
    #else
        #define ABEL_HAVE_ISNAN(x)  std::isnan(x)      /* declared in <cmath> */
        #define ABEL_HAVE_ISINF(x)  std::isinf(x)
    #endif
#endif

#if !defined(ABEL_HAVE_itoa_DECL) && !defined(ABEL_NO_HAVE_itoa_DECL)
    #if defined(ABEL_COMPILER_MSVC)
        #define ABEL_HAVE_itoa_DECL 1
    #else
        #define ABEL_NO_HAVE_itoa_DECL 1
    #endif
#endif

#if !defined(ABEL_HAVE_nanosleep_DECL) && !defined(ABEL_NO_HAVE_nanosleep_DECL)
    #if (defined(ABEL_PLATFORM_UNIX) && !defined(ABEL_PLATFORM_SONY)) || defined(ABEL_PLATFORM_IPHONE) || defined(ABEL_PLATFORM_OSX) || defined(ABEL_PLATFORM_SONY) || defined(CS_UNDEFINED_STRING)
        #define ABEL_HAVE_nanosleep_DECL 1
    #else
        #define ABEL_NO_HAVE_nanosleep_DECL 1
    #endif
#endif

#if !defined(ABEL_HAVE_utime_DECL) && !defined(ABEL_NO_HAVE_utime_DECL)
    #if defined(ABEL_PLATFORM_MICROSOFT)
        #define ABEL_HAVE_utime_DECL _utime
    #elif ABEL_PLATFORM_UNIX
        #define ABEL_HAVE_utime_DECL utime
    #else
        #define ABEL_NO_HAVE_utime_DECL 1
    #endif
#endif

#if !defined(ABEL_HAVE_ftruncate_DECL) && !defined(ABEL_NO_HAVE_ftruncate_DECL)
    #if !defined(__MINGW32__)
        #define ABEL_HAVE_ftruncate_DECL 1
    #else
        #define ABEL_NO_HAVE_ftruncate_DECL 1
    #endif
#endif

#if !defined(ABEL_HAVE_localtime_DECL) && !defined(ABEL_NO_HAVE_localtime_DECL)
    #define ABEL_HAVE_localtime_DECL 1
#endif

#if !defined(ABEL_HAVE_pthread_getattr_np_DECL) && !defined(ABEL_NO_HAVE_pthread_getattr_np_DECL)
    #if defined(ABEL_PLATFORM_LINUX)
        #define ABEL_HAVE_pthread_getattr_np_DECL 1
    #else
        #define ABEL_NO_HAVE_pthread_getattr_np_DECL 1
    #endif
#endif



/* ABEL_HAVE_XXX_IMPL*/

#if !defined(ABEL_HAVE_WCHAR_IMPL) && !defined(ABEL_NO_HAVE_WCHAR_IMPL)
    #if defined(ABEL_PLATFORM_DESKTOP)
        #define ABEL_HAVE_WCHAR_IMPL 1      /* Specifies if wchar_t string functions are provided, such as wcslen, wprintf, etc. Implies ABEL_HAVE_WCHAR_H */
    #else
        #define ABEL_NO_HAVE_WCHAR_IMPL 1
    #endif
#endif

#if !defined(ABEL_HAVE_getenv_IMPL) && !defined(ABEL_NO_HAVE_getenv_IMPL)
    #if (defined(ABEL_PLATFORM_DESKTOP) || defined(ABEL_PLATFORM_UNIX)) && !defined(ABEL_PLATFORM_WINRT)
        #define ABEL_HAVE_getenv_IMPL 1
    #else
        #define ABEL_NO_HAVE_getenv_IMPL 1
    #endif
#endif

#if !defined(ABEL_HAVE_setenv_IMPL) && !defined(ABEL_NO_HAVE_setenv_IMPL)
    #if defined(ABEL_PLATFORM_UNIX) && defined(ABEL_PLATFORM_POSIX)
        #define ABEL_HAVE_setenv_IMPL 1
    #else
        #define ABEL_NO_HAVE_setenv_IMPL 1
    #endif
#endif

#if !defined(ABEL_HAVE_unsetenv_IMPL) && !defined(ABEL_NO_HAVE_unsetenv_IMPL)
    #if defined(ABEL_PLATFORM_UNIX) && defined(ABEL_PLATFORM_POSIX)
        #define ABEL_HAVE_unsetenv_IMPL 1
    #else
        #define ABEL_NO_HAVE_unsetenv_IMPL 1
    #endif
#endif

#if !defined(ABEL_HAVE_putenv_IMPL) && !defined(ABEL_NO_HAVE_putenv_IMPL)
    #if (defined(ABEL_PLATFORM_DESKTOP) || defined(ABEL_PLATFORM_UNIX)) && !defined(ABEL_PLATFORM_WINRT)
        #define ABEL_HAVE_putenv_IMPL 1        /* With Microsoft compilers you may need to use _putenv, as they have deprecated putenv. */
    #else
        #define ABEL_NO_HAVE_putenv_IMPL 1
    #endif
#endif

#if !defined(ABEL_HAVE_time_IMPL) && !defined(ABEL_NO_HAVE_time_IMPL)
    #define ABEL_HAVE_time_IMPL 1
    #define ABEL_HAVE_clock_IMPL 1
#endif

// <cstdio> fopen()
#if !defined(ABEL_HAVE_fopen_IMPL) && !defined(ABEL_NO_HAVE_fopen_IMPL)
    #define ABEL_HAVE_fopen_IMPL 1  /* C FILE functionality such as fopen */
#endif

// <arpa/inet.h> inet_ntop()
#if !defined(ABEL_HAVE_inet_ntop_IMPL) && !defined(ABEL_NO_HAVE_inet_ntop_IMPL)
    #if (defined(ABEL_PLATFORM_UNIX) || defined(ABEL_PLATFORM_POSIX)) && !defined(ABEL_PLATFORM_SONY) && !defined(CS_UNDEFINED_STRING)
        #define ABEL_HAVE_inet_ntop_IMPL 1  /* This doesn't identify if the platform SDK has some alternative function that does the same thing; */
        #define ABEL_HAVE_inet_pton_IMPL 1  /* it identifies strictly the <arpa/inet.h> inet_ntop and inet_pton functions. For example, Microsoft has InetNtop in <Ws2tcpip.h> */
    #else
        #define ABEL_NO_HAVE_inet_ntop_IMPL 1
        #define ABEL_NO_HAVE_inet_pton_IMPL 1
    #endif
#endif

// <time.h> clock_gettime()
#if !defined(ABEL_HAVE_clock_gettime_IMPL) && !defined(ABEL_NO_HAVE_clock_gettime_IMPL)
    #if defined(ABEL_PLATFORM_LINUX) || defined(__CYGWIN__) || (defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)) || (defined(ABEL_PLATFORM_POSIX) && defined(_CPPLIB_VER) /*Dinkumware*/)
        #define ABEL_HAVE_clock_gettime_IMPL 1 /* You need to link the 'rt' library to get this */
    #else
        #define ABEL_NO_HAVE_clock_gettime_IMPL 1
    #endif
#endif

#if !defined(ABEL_HAVE_getcwd_IMPL) && !defined(ABEL_NO_HAVE_getcwd_IMPL)
    #if (defined(ABEL_PLATFORM_DESKTOP) || defined(ABEL_PLATFORM_UNIX)) && !defined(ABEL_PLATFORM_ANDROID) && !defined(ABEL_PLATFORM_WINRT)
        #define ABEL_HAVE_getcwd_IMPL 1       /* With Microsoft compilers you may need to use _getcwd, as they have deprecated getcwd. And in any case it's present at <direct.h> */
    #else
        #define ABEL_NO_HAVE_getcwd_IMPL 1
    #endif
#endif

#if !defined(ABEL_HAVE_tmpnam_IMPL) && !defined(ABEL_NO_HAVE_tmpnam_IMPL)
    #if (defined(ABEL_PLATFORM_DESKTOP) || defined(ABEL_PLATFORM_UNIX)) && !defined(ABEL_PLATFORM_ANDROID)
        #define ABEL_HAVE_tmpnam_IMPL 1
    #else
        #define ABEL_NO_HAVE_tmpnam_IMPL 1
    #endif
#endif

// nullptr, the built-in C++11 type.
// This ABEL_HAVE is deprecated, as ABEL_COMPILER_NO_NULLPTR is more appropriate, given that nullptr is a compiler-level feature and not a library feature.
#if !defined(ABEL_HAVE_nullptr_IMPL) && !defined(ABEL_NO_HAVE_nullptr_IMPL)
    #if defined(ABEL_COMPILER_NO_NULLPTR)
        #define ABEL_NO_HAVE_nullptr_IMPL 1
    #else
        #define ABEL_HAVE_nullptr_IMPL 1
    #endif
#endif

// <cstddef> std::nullptr_t
// Note that <cxxbase/nullptr.h> implements a portable nullptr implementation, but this
// ABEL_HAVE specifically refers to std::nullptr_t from the standard libraries.
#if !defined(ABEL_HAVE_nullptr_t_IMPL) && !defined(ABEL_NO_HAVE_nullptr_t_IMPL)
    #if defined(ABEL_COMPILER_CPP11_ENABLED)
        // VS2010+ with its default Dinkumware standard library.
        #if defined(_MSC_VER) && (_MSC_VER >= 1600) && defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY)
            #define ABEL_HAVE_nullptr_t_IMPL 1

        #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) // clang/llvm libc++
            #define ABEL_HAVE_nullptr_t_IMPL 1

        #elif defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) // GNU libstdc++
            // Unfortunately __GLIBCXX__ date values don't go strictly in version ordering.
            #if (__GLIBCXX__ >= 20110325) && (__GLIBCXX__ != 20120702) && (__GLIBCXX__ != 20110428)
                #define ABEL_HAVE_nullptr_t_IMPL 1
            #else
                #define ABEL_NO_HAVE_nullptr_t_IMPL 1
            #endif

        // We simply assume that the standard library (e.g. Dinkumware) provides std::nullptr_t.
        #elif defined(__clang__)
            #define ABEL_HAVE_nullptr_t_IMPL 1

        // With GCC compiler >= 4.6, std::nullptr_t is always defined in <cstddef>, in practice.
        #elif defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4006)
            #define ABEL_HAVE_nullptr_t_IMPL 1

        // The EDG compiler provides nullptr, but uses an older standard library that doesn't support std::nullptr_t.
        #elif defined(__EDG_VERSION__) && (__EDG_VERSION__ >= 403)
            #define ABEL_HAVE_nullptr_t_IMPL 1

        #else
            #define ABEL_NO_HAVE_nullptr_t_IMPL 1
        #endif
    #else
        #define ABEL_NO_HAVE_nullptr_t_IMPL 1
    #endif
#endif

// <exception> std::terminate
#if !defined(ABEL_HAVE_std_terminate_IMPL) && !defined(ABEL_NO_HAVE_std_terminate_IMPL)
    #if !defined(ABEL_PLATFORM_IPHONE) && !defined(ABEL_PLATFORM_ANDROID)
        #define ABEL_HAVE_std_terminate_IMPL 1 /* iOS doesn't appear to provide an implementation for std::terminate under the armv6 target. */
    #else
        #define ABEL_NO_HAVE_std_terminate_IMPL 1
    #endif
#endif

// <iterator>: std::begin, std::end, std::prev, std::next, std::move_iterator.
#if !defined(ABEL_HAVE_CPP11_ITERATOR_IMPL) && !defined(ABEL_NO_HAVE_CPP11_ITERATOR_IMPL)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) && !(defined(_HAS_CPP0X) && _HAS_CPP0X) // Dinkumware. VS2010+
        #define ABEL_HAVE_CPP11_ITERATOR_IMPL 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4006)
        #define ABEL_HAVE_CPP11_ITERATOR_IMPL 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_ITERATOR_IMPL 1
    #else
        #define ABEL_NO_HAVE_CPP11_ITERATOR_IMPL 1
    #endif
#endif

// <memory>: std::weak_ptr, std::shared_ptr, std::unique_ptr, std::bad_weak_ptr, std::owner_less
#if !defined(ABEL_HAVE_CPP11_SMART_POINTER_IMPL) && !defined(ABEL_NO_HAVE_CPP11_SMART_POINTER_IMPL)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) && !(defined(_HAS_CPP0X) && _HAS_CPP0X) // Dinkumware. VS2010+
        #define ABEL_HAVE_CPP11_SMART_POINTER_IMPL 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4004)
        #define ABEL_HAVE_CPP11_SMART_POINTER_IMPL 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_SMART_POINTER_IMPL 1
    #else
        #define ABEL_NO_HAVE_CPP11_SMART_POINTER_IMPL 1
    #endif
#endif

// <functional>: std::function, std::mem_fn, std::bad_function_call, std::is_bind_expression, std::is_placeholder, std::reference_wrapper, std::hash, std::bind, std::ref, std::cref.
#if !defined(ABEL_HAVE_CPP11_FUNCTIONAL_IMPL) && !defined(ABEL_NO_HAVE_CPP11_FUNCTIONAL_IMPL)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) && !(defined(_HAS_CPP0X) && _HAS_CPP0X) // Dinkumware. VS2010+
        #define ABEL_HAVE_CPP11_FUNCTIONAL_IMPL 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4004)
        #define ABEL_HAVE_CPP11_FUNCTIONAL_IMPL 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_FUNCTIONAL_IMPL 1
    #else
        #define ABEL_NO_HAVE_CPP11_FUNCTIONAL_IMPL 1
    #endif
#endif

// <exception> std::current_exception, std::rethrow_exception, std::exception_ptr, std::make_exception_ptr
#if !defined(ABEL_HAVE_CPP11_EXCEPTION_IMPL) && !defined(ABEL_NO_HAVE_CPP11_EXCEPTION_IMPL)
    #if defined(ABEL_HAVE_DINKUMWARE_CPP_LIBRARY) && (_CPPLIB_VER >= 520) && !(defined(_HAS_CPP0X) && _HAS_CPP0X) // Dinkumware. VS2010+
        #define ABEL_HAVE_CPP11_EXCEPTION_IMPL 1
    #elif defined(ABEL_COMPILER_CPP11_ENABLED) && defined(ABEL_HAVE_LIBSTDCPP_LIBRARY) && defined(ABEL_COMPILER_GNUC) && (ABEL_COMPILER_VERSION >= 4004)
        #define ABEL_HAVE_CPP11_EXCEPTION_IMPL 1
    #elif defined(ABEL_HAVE_LIBCPP_LIBRARY) && (_LIBCPP_VERSION >= 1)
        #define ABEL_HAVE_CPP11_EXCEPTION_IMPL 1
    #else
        #define ABEL_NO_HAVE_CPP11_EXCEPTION_IMPL 1
    #endif
#endif

#endif //ABEL_BASE_PROFILE_HAVE_H_
