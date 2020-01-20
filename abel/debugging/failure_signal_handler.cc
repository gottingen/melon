//
//

#include <abel/debugging/failure_signal_handler.h>

#include <abel/base/profile.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef ABEL_HAVE_MMAP
#include <sys/mman.h>
#endif

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <abel/base/profile.h>
#include <abel/base/internal/raw_logging.h>
#include <abel/base/internal/sysinfo.h>
#include <abel/debugging/internal/examine_stack.h>
#include <abel/debugging/stacktrace.h>

#ifndef _WIN32
#define ABEL_HAVE_SIGACTION
#endif

namespace abel {


ABEL_CONST_INIT static FailureSignalHandlerOptions fsh_options;

// Resets the signal handler for signo to the default action for that
// signal, then raises the signal.
static void RaiseToDefaultHandler(int signo) {
  signal(signo, SIG_DFL);
  raise(signo);
}

struct FailureSignalData {
  const int signo;
  const char* const as_string;
#ifdef ABEL_HAVE_SIGACTION
  struct sigaction previous_action;
  // StructSigaction is used to silence -Wmissing-field-initializers.
  using StructSigaction = struct sigaction;
  #define FSD_PREVIOUS_INIT FailureSignalData::StructSigaction()
#else
  void (*previous_handler)(int);
  #define FSD_PREVIOUS_INIT SIG_DFL
#endif
};

ABEL_CONST_INIT static FailureSignalData failure_signal_data[] = {
    {SIGSEGV, "SIGSEGV", FSD_PREVIOUS_INIT},
    {SIGILL, "SIGILL", FSD_PREVIOUS_INIT},
    {SIGFPE, "SIGFPE", FSD_PREVIOUS_INIT},
    {SIGABRT, "SIGABRT", FSD_PREVIOUS_INIT},
    {SIGTERM, "SIGTERM", FSD_PREVIOUS_INIT},
#ifndef _WIN32
    {SIGBUS, "SIGBUS", FSD_PREVIOUS_INIT},
    {SIGTRAP, "SIGTRAP", FSD_PREVIOUS_INIT},
#endif
};

#undef FSD_PREVIOUS_INIT

static void RaiseToPreviousHandler(int signo) {
  // Search for the previous handler.
  for (const auto& it : failure_signal_data) {
    if (it.signo == signo) {
#ifdef ABEL_HAVE_SIGACTION
      sigaction(signo, &it.previous_action, nullptr);
#else
      signal(signo, it.previous_handler);
#endif
      raise(signo);
      return;
    }
  }

  // Not found, use the default handler.
  RaiseToDefaultHandler(signo);
}

namespace debugging_internal {

const char* FailureSignalToString(int signo) {
  for (const auto& it : failure_signal_data) {
    if (it.signo == signo) {
      return it.as_string;
    }
  }
  return "";
}

}  // namespace debugging_internal

#ifndef _WIN32

static bool SetupAlternateStackOnce() {
#if defined(__wasm__) || defined (__asjms__)
  const size_t page_mask = getpagesize() - 1;
#else
  const size_t page_mask = sysconf(_SC_PAGESIZE) - 1;
#endif
  size_t stack_size = (std::max(SIGSTKSZ, 65536) + page_mask) & ~page_mask;
#if defined(ADDRESS_SANITIZER) || defined(MEMORY_SANITIZER) || \
    defined(THREAD_SANITIZER)
  // Account for sanitizer instrumentation requiring additional stack space.
  stack_size *= 5;
#endif

  stack_t sigstk;
  memset(&sigstk, 0, sizeof(sigstk));
  sigstk.ss_size = stack_size;

#ifdef ABEL_HAVE_MMAP
#ifndef MAP_STACK
#define MAP_STACK 0
#endif
#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif
  sigstk.ss_sp = mmap(nullptr, sigstk.ss_size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
  if (sigstk.ss_sp == MAP_FAILED) {
    ABEL_RAW_LOG(FATAL, "mmap() for alternate signal stack failed");
  }
#else
  sigstk.ss_sp = malloc(sigstk.ss_size);
  if (sigstk.ss_sp == nullptr) {
    ABEL_RAW_LOG(FATAL, "malloc() for alternate signal stack failed");
  }
#endif

  if (sigaltstack(&sigstk, nullptr) != 0) {
    ABEL_RAW_LOG(FATAL, "sigaltstack() failed with errno=%d", errno);
  }
  return true;
}

#endif

#ifdef ABEL_HAVE_SIGACTION

// Sets up an alternate stack for signal handlers once.
// Returns the appropriate flag for sig_action.sa_flags
// if the system supports using an alternate stack.
static int MaybeSetupAlternateStack() {
#ifndef _WIN32
  ABEL_ATTRIBUTE_UNUSED static const bool kOnce = SetupAlternateStackOnce();
  return SA_ONSTACK;
#else
  return 0;
#endif
}

static void InstallOneFailureHandler(FailureSignalData* data,
                                     void (*handler)(int, siginfo_t*, void*)) {
  struct sigaction act;
  memset(&act, 0, sizeof(act));
  sigemptyset(&act.sa_mask);
  act.sa_flags |= SA_SIGINFO;
  // SA_NODEFER is required to handle SIGABRT from
  // ImmediateAbortSignalHandler().
  act.sa_flags |= SA_NODEFER;
  if (fsh_options.use_alternate_stack) {
    act.sa_flags |= MaybeSetupAlternateStack();
  }
  act.sa_sigaction = handler;
  ABEL_RAW_CHECK(sigaction(data->signo, &act, &data->previous_action) == 0,
                 "sigaction() failed");
}

#else

static void InstallOneFailureHandler(FailureSignalData* data,
                                     void (*handler)(int)) {
  data->previous_handler = signal(data->signo, handler);
  ABEL_RAW_CHECK(data->previous_handler != SIG_ERR, "signal() failed");
}

#endif

static void WriteToStderr(const char* data) {
  int old_errno = errno;
  abel::raw_logging_internal::SafeWriteToStderr(data, strlen(data));
  errno = old_errno;
}

static void WriteSignalMessage(int signo, void (*writerfn)(const char*)) {
  char buf[64];
  const char* const signal_string =
      debugging_internal::FailureSignalToString(signo);
  if (signal_string != nullptr && signal_string[0] != '\0') {
    snprintf(buf, sizeof(buf), "*** %s received at time=%ld ***\n",
             signal_string,
             static_cast<long>(time(nullptr)));  // NOLINT(runtime/int)
  } else {
    snprintf(buf, sizeof(buf), "*** signal %d received at time=%ld ***\n",
             signo, static_cast<long>(time(nullptr)));  // NOLINT(runtime/int)
  }
  writerfn(buf);
}

// `void*` might not be big enough to store `void(*)(const char*)`.
struct WriterFnStruct {
  void (*writerfn)(const char*);
};

// Many of the abel::debugging_internal::Dump* functions in
// examine_stack.h take a writer function pointer that has a void* arg
// for historical reasons. failure_signal_handler_writer only takes a
// data pointer. This function converts between these types.
static void WriterFnWrapper(const char* data, void* arg) {
  static_cast<WriterFnStruct*>(arg)->writerfn(data);
}

// Convenient wrapper around DumpPCAndFrameSizesAndStackTrace() for signal
// handlers. "noinline" so that GetStackFrames() skips the top-most stack
// frame for this function.
ABEL_NO_INLINE static void WriteStackTrace(
    void* ucontext, bool symbolize_stacktrace,
    void (*writerfn)(const char*, void*), void* writerfn_arg) {
  constexpr int kNumStackFrames = 32;
  void* stack[kNumStackFrames];
  int frame_sizes[kNumStackFrames];
  int min_dropped_frames;
  int depth = abel::GetStackFramesWithContext(
      stack, frame_sizes, kNumStackFrames,
      1,  // Do not include this function in stack trace.
      ucontext, &min_dropped_frames);
  abel::debugging_internal::DumpPCAndFrameSizesAndStackTrace(
      abel::debugging_internal::GetProgramCounter(ucontext), stack, frame_sizes,
      depth, min_dropped_frames, symbolize_stacktrace, writerfn, writerfn_arg);
}

// Called by AbelFailureSignalHandler() to write the failure info. It is
// called once with writerfn set to WriteToStderr() and then possibly
// with writerfn set to the user provided function.
static void WriteFailureInfo(int signo, void* ucontext,
                             void (*writerfn)(const char*)) {
  WriterFnStruct writerfn_struct{writerfn};
  WriteSignalMessage(signo, writerfn);
  WriteStackTrace(ucontext, fsh_options.symbolize_stacktrace, WriterFnWrapper,
                  &writerfn_struct);
}

// abel::sleep_for() can't be used here since abel_internal_sleep_for()
// may be overridden to do something that isn't async-signal-safe on
// some platforms.
static void PortableSleepForSeconds(int seconds) {
#ifdef _WIN32
  Sleep(seconds * 1000);
#else
  struct timespec sleep_time;
  sleep_time.tv_sec = seconds;
  sleep_time.tv_nsec = 0;
  while (nanosleep(&sleep_time, &sleep_time) != 0 && errno == EINTR) {}
#endif
}

#ifdef ABEL_HAVE_ALARM
// AbelFailureSignalHandler() installs this as a signal handler for
// SIGALRM, then sets an alarm to be delivered to the program after a
// set amount of time. If AbelFailureSignalHandler() hangs for more than
// the alarm timeout, ImmediateAbortSignalHandler() will abort the
// program.
static void ImmediateAbortSignalHandler(int) {
  RaiseToDefaultHandler(SIGABRT);
}
#endif

// abel::base_internal::GetTID() returns pid_t on most platforms, but
// returns abel::base_internal::pid_t on Windows.
using GetTidType = decltype(abel::base_internal::GetTID());
ABEL_CONST_INIT static std::atomic<GetTidType> failed_tid(0);

#ifndef ABEL_HAVE_SIGACTION
static void AbelFailureSignalHandler(int signo) {
  void* ucontext = nullptr;
#else
static void AbelFailureSignalHandler(int signo, siginfo_t*, void* ucontext) {
#endif

  const GetTidType this_tid = abel::base_internal::GetTID();
  GetTidType previous_failed_tid = 0;
  if (!failed_tid.compare_exchange_strong(
          previous_failed_tid, static_cast<intptr_t>(this_tid),
          std::memory_order_acq_rel, std::memory_order_relaxed)) {
    ABEL_RAW_LOG(
        ERROR,
        "signal %d raised at PC=%p while already in AbelFailureSignalHandler()",
        signo, abel::debugging_internal::GetProgramCounter(ucontext));
    if (this_tid != previous_failed_tid) {
      // Another thread is already in AbelFailureSignalHandler(), so wait
      // a bit for it to finish. If the other thread doesn't kill us,
      // we do so after sleeping.
      PortableSleepForSeconds(3);
      RaiseToDefaultHandler(signo);
      // The recursively raised signal may be blocked until we return.
      return;
    }
  }

#ifdef ABEL_HAVE_ALARM
  // Set an alarm to abort the program in case this code hangs or deadlocks.
  if (fsh_options.alarm_on_failure_secs > 0) {
    alarm(0);  // Cancel any existing alarms.
    signal(SIGALRM, ImmediateAbortSignalHandler);
    alarm(fsh_options.alarm_on_failure_secs);
  }
#endif

  // First write to stderr.
  WriteFailureInfo(signo, ucontext, WriteToStderr);

  // Riskier code (because it is less likely to be async-signal-safe)
  // goes after this point.
  if (fsh_options.writerfn != nullptr) {
    WriteFailureInfo(signo, ucontext, fsh_options.writerfn);
  }

  if (fsh_options.call_previous_handler) {
    RaiseToPreviousHandler(signo);
  } else {
    RaiseToDefaultHandler(signo);
  }
}

void InstallFailureSignalHandler(const FailureSignalHandlerOptions& options) {
  fsh_options = options;
  for (auto& it : failure_signal_data) {
    InstallOneFailureHandler(&it, AbelFailureSignalHandler);
  }
}


}  // namespace abel
