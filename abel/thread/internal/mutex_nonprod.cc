//

// Implementation of a small subset of mutex and cond_var functionality
// for platforms where the production implementation hasn't been fully
// ported yet.

#include <abel/thread/mutex.h>

#if defined(ABEL_INTERNAL_USE_NONPROD_MUTEX)

#if defined(_WIN32)
#include <chrono>  // NOLINT(build/c++11)
#else
#include <sys/time.h>
#include <time.h>
#endif

#include <algorithm>

#include <abel/log/abel_logging.h>
#include <abel/chrono/time.h>

namespace abel {

namespace thread_internal {

namespace {

// Return the current time plus the timeout.
abel::abel_time DeadlineFromTimeout(abel::duration timeout) {
  return abel::now() + timeout;
}

// Limit the deadline to a positive, 32-bit time_t value to accommodate
// implementation restrictions.  This also deals with infinite_past and
// infinite_future.
abel::abel_time LimitedDeadline(abel::abel_time deadline) {
  deadline = std::max(abel::from_time_t(0), deadline);
  deadline = std::min(deadline, abel::from_time_t(0x7fffffff));
  return deadline;
}

}  // namespace

#if defined(_WIN32)

mutex_impl::mutex_impl() {}

mutex_impl::~mutex_impl() {
  if (locked_) {
    std_mutex_.unlock();
  }
}

void mutex_impl::lock() {
  std_mutex_.lock();
  locked_ = true;
}

bool mutex_impl::try_lock() {
  bool locked = std_mutex_.try_lock();
  if (locked) locked_ = true;
  return locked;
}

void mutex_impl::unlock() {
  locked_ = false;
  released_.signal_all();
  std_mutex_.unlock();
}

cond_var_impl::cond_var_impl() {}

cond_var_impl::~cond_var_impl() {}

void cond_var_impl::signal() { std_cv_.notify_one(); }

void cond_var_impl::signal_all() { std_cv_.notify_all(); }

void cond_var_impl::wait(mutex_impl* mu) {
  mu->released_.signal_all();
  std_cv_.wait(mu->std_mutex_);
}

bool cond_var_impl::wait_with_deadline(mutex_impl* mu, abel::abel_time deadline) {
  mu->released_.signal_all();
  time_t when = to_time_t(deadline);
  int64_t nanos = to_int64_nanoseconds(deadline - abel::from_time_t(when));
  std::chrono::system_clock::time_point deadline_tp =
      std::chrono::system_clock::from_time_t(when) +
      std::chrono::duration_cast<std::chrono::system_clock::duration>(
          std::chrono::nanoseconds(nanos));
  auto deadline_since_epoch =
      std::chrono::duration_cast<std::chrono::duration<double>>(
          deadline_tp - std::chrono::system_clock::from_time_t(0));
  return std_cv_.wait_until(mu->std_mutex_, deadline_tp) ==
         std::cv_status::timeout;
}

#else  // ! _WIN32

mutex_impl::mutex_impl() {
  ABEL_RAW_CHECK(pthread_mutex_init(&pthread_mutex_, nullptr) == 0,
                 "pthread error");
}

mutex_impl::~mutex_impl() {
  if (locked_) {
    ABEL_RAW_CHECK(pthread_mutex_unlock(&pthread_mutex_) == 0, "pthread error");
  }
  ABEL_RAW_CHECK(pthread_mutex_destroy(&pthread_mutex_) == 0, "pthread error");
}

void mutex_impl::lock() {
  ABEL_RAW_CHECK(pthread_mutex_lock(&pthread_mutex_) == 0, "pthread error");
  locked_ = true;
}

bool mutex_impl::try_lock() {
  bool locked = (0 == pthread_mutex_trylock(&pthread_mutex_));
  if (locked) locked_ = true;
  return locked;
}

void mutex_impl::unlock() {
  locked_ = false;
  released_.signal_all();
  ABEL_RAW_CHECK(pthread_mutex_unlock(&pthread_mutex_) == 0, "pthread error");
}

cond_var_impl::cond_var_impl() {
  ABEL_RAW_CHECK(pthread_cond_init(&pthread_cv_, nullptr) == 0,
                 "pthread error");
}

cond_var_impl::~cond_var_impl() {
  ABEL_RAW_CHECK(pthread_cond_destroy(&pthread_cv_) == 0, "pthread error");
}

void cond_var_impl::signal() {
  ABEL_RAW_CHECK(pthread_cond_signal(&pthread_cv_) == 0, "pthread error");
}

void cond_var_impl::signal_all() {
  ABEL_RAW_CHECK(pthread_cond_broadcast(&pthread_cv_) == 0, "pthread error");
}

void cond_var_impl::wait(mutex_impl* mu) {
  mu->released_.signal_all();
  ABEL_RAW_CHECK(pthread_cond_wait(&pthread_cv_, &mu->pthread_mutex_) == 0,
                 "pthread error");
}

bool cond_var_impl::wait_with_deadline(mutex_impl* mu, abel::abel_time deadline) {
  mu->released_.signal_all();
  struct timespec ts = to_timespec(deadline);
  int rc = pthread_cond_timedwait(&pthread_cv_, &mu->pthread_mutex_, &ts);
  if (rc == ETIMEDOUT) return true;
  ABEL_RAW_CHECK(rc == 0, "pthread error");
  return false;
}

#endif  // ! _WIN32

void mutex_impl::await(const condition& cond) {
  if (cond.Eval()) return;
  released_.signal_all();
  do {
    released_.wait(this);
  } while (!cond.Eval());
}

bool mutex_impl::await_with_deadline(const condition& cond, abel::abel_time deadline) {
  if (cond.Eval()) return true;
  released_.signal_all();
  while (true) {
    if (released_.wait_with_deadline(this, deadline)) return false;
    if (cond.Eval()) return true;
  }
}

}  // namespace thread_internal

mutex::mutex() {}

mutex::~mutex() {}

void mutex::lock() { impl()->lock(); }

void mutex::unlock() { impl()->unlock(); }

bool mutex::try_lock() { return impl()->try_lock(); }

void mutex::reader_lock() { lock(); }

void mutex::reader_unlock() { unlock(); }

void mutex::await(const condition& cond) { impl()->await(cond); }

void mutex::lock_when(const condition& cond) {
  lock();
  await(cond);
}

bool mutex::await_with_deadline(const condition& cond, abel::abel_time deadline) {
  return impl()->await_with_deadline(
      cond, thread_internal::LimitedDeadline(deadline));
}

bool mutex::await_with_timeout(const condition& cond, abel::duration timeout) {
  return await_with_deadline(
      cond, thread_internal::DeadlineFromTimeout(timeout));
}

bool mutex::lock_when_with_deadline(const condition& cond, abel::abel_time deadline) {
  lock();
  return await_with_deadline(cond, deadline);
}

bool mutex::lock_when_with_timeout(const condition& cond, abel::duration timeout) {
  return lock_when_with_deadline(
      cond, thread_internal::DeadlineFromTimeout(timeout));
}

void mutex::reader_lock_when(const condition& cond) {
  reader_lock();
  await(cond);
}

bool mutex::reader_lock_when_with_timeout(const condition& cond,
                                      abel::duration timeout) {
  return lock_when_with_timeout(cond, timeout);
}
bool mutex::reader_lock_when_with_deadline(const condition& cond,
                                       abel::abel_time deadline) {
  return lock_when_with_deadline(cond, deadline);
}

void mutex::enable_debug_log(const char*) {}
void mutex::enable_invariant_debugging(void (*)(void*), void*) {}
void mutex::forget_dead_lock_info() {}
void mutex::assert_held() const {}
void mutex::assert_reader_held() const {}
void mutex::assert_not_held() const {}

cond_var::cond_var() {}

cond_var::~cond_var() {}

void cond_var::signal() { impl()->signal(); }

void cond_var::signal_all() { impl()->signal_all(); }

void cond_var::wait(mutex* mu) { return impl()->wait(mu->impl()); }

bool cond_var::wait_with_deadline(mutex* mu, abel::abel_time deadline) {
  return impl()->wait_with_deadline(
      mu->impl(), thread_internal::LimitedDeadline(deadline));
}

bool cond_var::wait_with_timeout(mutex* mu, abel::duration timeout) {
  return wait_with_deadline(mu, abel::now() + timeout);
}

void cond_var::enable_debug_log(const char*) {}

#ifdef THREAD_SANITIZER
extern "C" void __tsan_read1(void *addr);
#else
#define __tsan_read1(addr)  // do nothing if TSan not enabled
#endif

// A function that just returns its argument, dereferenced
static bool Dereference(void *arg) {
  // ThreadSanitizer does not instrument this file for memory accesses.
  // This function dereferences a user variable that can participate
  // in a data race, so we need to manually tell TSan about this memory access.
  __tsan_read1(arg);
  return *(static_cast<bool *>(arg));
}

condition::condition() {}   // null constructor, used for kTrue only
const condition condition::kTrue;

condition::condition(bool (*func)(void *), void *arg)
    : eval_(&call_void_ptr_function),
      function_(func),
      method_(nullptr),
      arg_(arg) {}

bool condition::call_void_ptr_function(const condition *c) {
  return (*c->function_)(c->arg_);
}

condition::condition(const bool *cond)
    : eval_(call_void_ptr_function),
      function_(Dereference),
      method_(nullptr),
      // const_cast is safe since Dereference does not modify arg
      arg_(const_cast<bool *>(cond)) {}

bool condition::Eval() const {
  // eval_ == null for kTrue
  return (this->eval_ == nullptr) || (*this->eval_)(this);
}

void register_symbolizer(bool (*)(const void*, char*, int)) {}


}  // namespace abel

#endif
