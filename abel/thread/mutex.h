// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

// -----------------------------------------------------------------------------
// mutex.h
// -----------------------------------------------------------------------------
//
// This header file defines a `mutex` -- a mutually exclusive lock -- and the
// most common type of synchronization primitive for facilitating locks on
// shared resources. A mutex is used to prevent multiple threads from accessing
// and/or writing to a shared resource concurrently.
//
// Unlike a `std::mutex`, the abel `mutex` provides the following additional
// features:
//   * Conditional predicates intrinsic to the `mutex` object
//   * Shared/reader locks, in addition to standard exclusive/writer locks
//   * Deadlock detection and debug support.
//
// The following helper classes are also defined within this file:
//
//  mutex_lock - An RAII wrapper to acquire and release a `mutex` for exclusive/
//              write access within the current scope.
//  reader_mutex_lock
//            - An RAII wrapper to acquire and release a `mutex` for shared/read
//              access within the current scope.
//
//  writer_mutex_lock
//            - Alias for `mutex_lock` above, designed for use in distinguishing
//              reader and writer locks within code.
//
// In addition to simple mutex locks, this file also defines ways to perform
// locking under certain conditions.
//
//  condition   - (Preferred) Used to wait for a particular predicate that
//                depends on state protected by the `mutex` to become true.
//  cond_var     - A lower-level variant of `condition` that relies on
//                application code to explicitly signal the `cond_var` when
//                a condition has been met.
//
// See below for more information on using `condition` or `cond_var`.
//
// Mutexes and mutex behavior can be quite complicated. The information within
// this header file is limited, as a result. Please consult the mutex guide for
// more complete information and examples.

#ifndef ABEL_SYNCHRONIZATION_MUTEX_H_
#define ABEL_SYNCHRONIZATION_MUTEX_H_

#include <atomic>
#include <cstdint>
#include <string>
#include "abel/base/const_init.h"
#include "abel/meta/type_traits.h"
//#include "abel/memory/internal/low_level_alloc.h"
#include "abel/thread/internal/thread_identity.h"
#include "abel/thread/internal/tsan_mutex_interface.h"
#include "abel/base/profile.h"
#include "abel/thread/thread_annotations.h"
#include "abel/thread/internal/kernel_timeout.h"
#include "abel/thread/internal/per_thread_sem.h"
#include "abel/chrono/time.h"

namespace abel {


class condition;

struct synch_wait_params;

// -----------------------------------------------------------------------------
// mutex
// -----------------------------------------------------------------------------
//
// A `mutex` is a non-reentrant (aka non-recursive) Mutually Exclusive lock
// on some resource, typically a variable or data structure with associated
// invariants. Proper usage of mutexes prevents concurrent access by different
// threads to the same resource.
//
// A `mutex` has two basic operations: `mutex::lock()` and `mutex::unlock()`.
// The `lock()` operation *acquires* a `mutex` (in a state known as an
// *exclusive* -- or write -- lock), while the `unlock()` operation *releases* a
// mutex. During the span of time between the lock() and unlock() operations,
// a mutex is said to be *held*. By design all mutexes support exclusive/write
// locks, as this is the most common way to use a mutex.
//
// The `mutex` state machine for basic lock/unlock operations is quite simple:
//
// |                | lock()     | unlock() |
// |----------------+------------+----------|
// | Free           | Exclusive  | invalid  |
// | Exclusive      | blocks     | Free     |
//
// Attempts to `unlock()` must originate from the thread that performed the
// corresponding `lock()` operation.
//
// An "invalid" operation is disallowed by the API. The `mutex` implementation
// is allowed to do anything on an invalid call, including but not limited to
// crashing with a useful error message, silently succeeding, or corrupting
// data structures. In debug mode, the implementation attempts to crash with a
// useful error message.
//
// `mutex` is not guaranteed to be "fair" in prioritizing waiting threads; it
// is, however, approximately fair over long periods, and starvation-free for
// threads at the same priority.
//
// The lock/unlock primitives are now annotated with lock annotations
// defined in (base/thread_annotations.h). When writing multi-threaded code,
// you should use lock annotations whenever possible to document your lock
// synchronization policy. Besides acting as documentation, these annotations
// also help compilers or static analysis tools to identify and warn about
// issues that could potentially result in race conditions and deadlocks.
//
// For more information about the lock annotations, please see
// [Thread Safety Analysis](http://clang.llvm.org/docs/ThreadSafetyAnalysis.html)
// in the Clang documentation.
//
// See also `mutex_lock`, below, for scoped `mutex` acquisition.

class ABEL_LOCKABLE mutex {
  public:
    // Creates a `mutex` that is not held by anyone. This constructor is
    // typically used for Mutexes allocated on the heap or the stack.
    //
    // To create `mutex` instances with static storage duration
    // (e.g. a namespace-scoped or global variable), see
    // `mutex::mutex(abel::kConstInit)` below instead.
    mutex();

    // Creates a mutex with static storage duration.  A global variable
    // constructed this way avoids the lifetime issues that can occur on program
    // startup and shutdown.  (See abel/base/const_init.h.)
    //
    // For Mutexes allocated on the heap and stack, instead use the default
    // constructor, which can interact more fully with the thread sanitizer.
    //
    // Example usage:
    //   namespace foo {
    //   ABEL_CONST_INIT mutex mu(abel::kConstInit);
    //   }
    explicit constexpr mutex(abel::const_init_type);

    ~mutex();

    // mutex::lock()
    //
    // Blocks the calling thread, if necessary, until this `mutex` is free, and
    // then acquires it exclusively. (This lock is also known as a "write lock.")
    void lock() ABEL_EXCLUSIVE_LOCK_FUNCTION();

    // mutex::unlock()
    //
    // Releases this `mutex` and returns it from the exclusive/write state to the
    // free state. Caller must hold the `mutex` exclusively.
    void unlock() ABEL_UNLOCK_FUNCTION();

    // mutex::try_lock()
    //
    // If the mutex can be acquired without blocking, does so exclusively and
    // returns `true`. Otherwise, returns `false`. Returns `true` with high
    // probability if the `mutex` was free.
    bool try_lock() ABEL_EXCLUSIVE_TRYLOCK_FUNCTION(true);

    // mutex::assert_held()
    //
    // Return immediately if this thread holds the `mutex` exclusively (in write
    // mode). Otherwise, may report an error (typically by crashing with a
    // diagnostic), or may return immediately.
    void assert_held() const ABEL_ASSERT_EXCLUSIVE_LOCK();

    // ---------------------------------------------------------------------------
    // Reader-Writer Locking
    // ---------------------------------------------------------------------------

    // A mutex can also be used as a starvation-free reader-writer lock.
    // Neither read-locks nor write-locks are reentrant/recursive to avoid
    // potential client programming errors.
    //
    // The mutex API provides `Writer*()` aliases for the existing `lock()`,
    // `unlock()` and `try_lock()` methods for use within applications mixing
    // reader/writer locks. Using `Reader*()` and `Writer*()` operations in this
    // manner can make locking behavior clearer when mixing read and write modes.
    //
    // Introducing reader locks necessarily complicates the `mutex` state
    // machine somewhat. The table below illustrates the allowed state transitions
    // of a mutex in such cases. Note that reader_lock() may block even if the lock
    // is held in shared mode; this occurs when another thread is blocked on a
    // call to writer_lock().
    //
    // ---------------------------------------------------------------------------
    //     Operation: writer_lock() unlock()  reader_lock()           reader_unlock()
    // ---------------------------------------------------------------------------
    // State
    // ---------------------------------------------------------------------------
    // Free           Exclusive    invalid   Shared(1)              invalid
    // Shared(1)      blocks       invalid   Shared(2) or blocks    Free
    // Shared(n) n>1  blocks       invalid   Shared(n+1) or blocks  Shared(n-1)
    // Exclusive      blocks       Free      blocks                 invalid
    // ---------------------------------------------------------------------------
    //
    // In comments below, "shared" refers to a state of Shared(n) for any n > 0.

    // mutex::reader_lock()
    //
    // Blocks the calling thread, if necessary, until this `mutex` is either free,
    // or in shared mode, and then acquires a share of it. Note that
    // `reader_lock()` will block if some other thread has an exclusive/writer lock
    // on the mutex.

    void reader_lock() ABEL_SHARED_LOCK_FUNCTION();

    // mutex::reader_unlock()
    //
    // Releases a read share of this `mutex`. `reader_unlock` may return a mutex to
    // the free state if this thread holds the last reader lock on the mutex. Note
    // that you cannot call `reader_unlock()` on a mutex held in write mode.
    void reader_unlock() ABEL_UNLOCK_FUNCTION();

    // mutex::reader_try_lock()
    //
    // If the mutex can be acquired without blocking, acquires this mutex for
    // shared access and returns `true`. Otherwise, returns `false`. Returns
    // `true` with high probability if the `mutex` was free or shared.
    bool reader_try_lock() ABEL_SHARED_TRYLOCK_FUNCTION(true);

    // mutex::assert_reader_held()
    //
    // Returns immediately if this thread holds the `mutex` in at least shared
    // mode (read mode). Otherwise, may report an error (typically by
    // crashing with a diagnostic), or may return immediately.
    void assert_reader_held() const ABEL_ASSERT_SHARED_LOCK();

    // mutex::writer_lock()
    // mutex::WriterUnlock()
    // mutex::WriterTryLock()
    //
    // Aliases for `mutex::lock()`, `mutex::unlock()`, and `mutex::try_lock()`.
    //
    // These methods may be used (along with the complementary `Reader*()`
    // methods) to distingish simple exclusive `mutex` usage (`lock()`,
    // etc.) from reader/writer lock usage.
    void writer_lock() ABEL_EXCLUSIVE_LOCK_FUNCTION() { this->lock(); }

    void writer_unlock() ABEL_UNLOCK_FUNCTION() { this->unlock(); }

    bool writer_try_lock() ABEL_EXCLUSIVE_TRYLOCK_FUNCTION(true) {
        return this->try_lock();
    }

    // ---------------------------------------------------------------------------
    // Conditional Critical Regions
    // ---------------------------------------------------------------------------

    // Conditional usage of a `mutex` can occur using two distinct paradigms:
    //
    //   * Use of `mutex` member functions with `condition` objects.
    //   * Use of the separate `cond_var` abstraction.
    //
    // In general, prefer use of `condition` and the `mutex` member functions
    // listed below over `cond_var`. When there are multiple threads waiting on
    // distinctly different conditions, however, a battery of `cond_var`s may be
    // more efficient. This section discusses use of `condition` objects.
    //
    // `mutex` contains member functions for performing lock operations only under
    // certain conditions, of class `condition`. For correctness, the `condition`
    // must return a boolean that is a pure function, only of state protected by
    // the `mutex`. The condition must be invariant w.r.t. environmental state
    // such as thread, cpu id, or time, and must be `noexcept`. The condition will
    // always be invoked with the mutex held in at least read mode, so you should
    // not block it for long periods or sleep it on a timer.
    //
    // Since a condition must not depend directly on the current time, use
    // `*WithTimeout()` member function variants to make your condition
    // effectively true after a given duration, or `*WithDeadline()` variants to
    // make your condition effectively true after a given time.
    //
    // The condition function should have no side-effects aside from debug
    // logging; as a special exception, the function may acquire other mutexes
    // provided it releases all those that it acquires.  (This exception was
    // required to allow logging.)

    // mutex::await()
    //
    // Unlocks this `mutex` and blocks until simultaneously both `cond` is `true`
    // and this `mutex` can be reacquired, then reacquires this `mutex` in the
    // same mode in which it was previously held. If the condition is initially
    // `true`, `await()` *may* skip the release/re-acquire step.
    //
    // `await()` requires that this thread holds this `mutex` in some mode.
    void await(const condition &cond);

    // mutex::lock_when()
    // mutex::reader_lock_when()
    // mutex::writer_lock_when()
    //
    // Blocks until simultaneously both `cond` is `true` and this `mutex` can
    // be acquired, then atomically acquires this `mutex`. `lock_when()` is
    // logically equivalent to `*lock(); await();` though they may have different
    // performance characteristics.
    void lock_when(const condition &cond) ABEL_EXCLUSIVE_LOCK_FUNCTION();

    void reader_lock_when(const condition &cond) ABEL_SHARED_LOCK_FUNCTION();

    void writer_lock_when(const condition &cond) ABEL_EXCLUSIVE_LOCK_FUNCTION() {
        this->lock_when(cond);
    }

    // ---------------------------------------------------------------------------
    // mutex Variants with Timeouts/Deadlines
    // ---------------------------------------------------------------------------

    // mutex::await_with_timeout()
    // mutex::await_with_deadline()
    //
    // If `cond` is initially true, do nothing, or act as though `cond` is
    // initially false.
    //
    // If `cond` is initially false, unlock this `mutex` and block until
    // simultaneously:
    //   - either `cond` is true or the {timeout has expired, deadline has passed}
    //     and
    //   - this `mutex` can be reacquired,
    // then reacquire this `mutex` in the same mode in which it was previously
    // held, returning `true` iff `cond` is `true` on return.
    //
    // Deadlines in the past are equivalent to an immediate deadline.
    // Negative timeouts are equivalent to a zero timeout.
    //
    // This method requires that this thread holds this `mutex` in some mode.
    bool await_with_timeout(const condition &cond, abel::duration timeout);

    bool await_with_deadline(const condition &cond, abel::time_point deadline);

    // mutex::lock_when_with_timeout()
    // mutex::reader_lock_when_with_timeout()
    // mutex::writer_lock_when_with_timeout()
    //
    // Blocks until simultaneously both:
    //   - either `cond` is `true` or the timeout has expired, and
    //   - this `mutex` can be acquired,
    // then atomically acquires this `mutex`, returning `true` iff `cond` is
    // `true` on return.
    //
    // Negative timeouts are equivalent to a zero timeout.
    bool lock_when_with_timeout(const condition &cond, abel::duration timeout)
    ABEL_EXCLUSIVE_LOCK_FUNCTION();

    bool reader_lock_when_with_timeout(const condition &cond, abel::duration timeout)
    ABEL_SHARED_LOCK_FUNCTION();

    bool writer_lock_when_with_timeout(const condition &cond, abel::duration timeout)
    ABEL_EXCLUSIVE_LOCK_FUNCTION() {
        return this->lock_when_with_timeout(cond, timeout);
    }

    // mutex::lock_when_with_deadline()
    // mutex::reader_lock_when_with_deadline()
    // mutex::writer_lock_when_with_deadline()
    //
    // Blocks until simultaneously both:
    //   - either `cond` is `true` or the deadline has been passed, and
    //   - this `mutex` can be acquired,
    // then atomically acquires this mutex, returning `true` iff `cond` is `true`
    // on return.
    //
    // Deadlines in the past are equivalent to an immediate deadline.
    bool lock_when_with_deadline(const condition &cond, abel::time_point deadline)
    ABEL_EXCLUSIVE_LOCK_FUNCTION();

    bool reader_lock_when_with_deadline(const condition &cond, abel::time_point deadline)
    ABEL_SHARED_LOCK_FUNCTION();

    bool writer_lock_when_with_deadline(const condition &cond, abel::time_point deadline)
    ABEL_EXCLUSIVE_LOCK_FUNCTION() {
        return this->lock_when_with_deadline(cond, deadline);
    }

    // ---------------------------------------------------------------------------
    // Debug Support: Invariant Checking, Deadlock Detection, Logging.
    // ---------------------------------------------------------------------------

    // mutex::enable_invariant_debugging()
    //
    // If `invariant`!=null and if invariant debugging has been enabled globally,
    // cause `(*invariant)(arg)` to be called at moments when the invariant for
    // this `mutex` should hold (for example: just after acquire, just before
    // release).
    //
    // The routine `invariant` should have no side-effects since it is not
    // guaranteed how many times it will be called; it should check the invariant
    // and crash if it does not hold. Enabling global invariant debugging may
    // substantially reduce `mutex` performance; it should be set only for
    // non-production runs.  Optimization options may also disable invariant
    // checks.
    void enable_invariant_debugging(void (*invariant)(void *), void *arg);

    // mutex::enable_debug_log()
    //
    // Cause all subsequent uses of this `mutex` to be logged via
    // `DLOG_INFO`. Log entries are tagged with `name` if no previous
    // call to `enable_invariant_debugging()` or `enable_debug_log()` has been made.
    //
    // Note: This method substantially reduces `mutex` performance.
    void enable_debug_log(const char *name);

    // Deadlock detection

    // mutex::forget_dead_lock_info()
    //
    // Forget any deadlock-detection information previously gathered
    // about this `mutex`. Call this method in debug mode when the lock ordering
    // of a `mutex` changes.
    void forget_dead_lock_info();

    // mutex::assert_not_held()
    //
    // Return immediately if this thread does not hold this `mutex` in any
    // mode; otherwise, may report an error (typically by crashing with a
    // diagnostic), or may return immediately.
    //
    // Currently this check is performed only if all of:
    //    - in debug mode
    //    - set_mutex_deadlock_detection_mode() has been set to kReport or kAbort
    //    - number of locks concurrently held by this thread is not large.
    // are true.
    void assert_not_held() const;

    // Special cases.

    // A `mu_how` is a constant that indicates how a lock should be acquired.
    // Internal implementation detail.  Clients should ignore.
    typedef const struct mu_how_s *mu_how;

    // mutex::internal_attempt_to_use_mutex_in_fatal_signal_handler()
    //
    // Causes the `mutex` implementation to prepare itself for re-entry caused by
    // future use of `mutex` within a fatal signal handler. This method is
    // intended for use only for last-ditch attempts to log crash information.
    // It does not guarantee that attempts to use Mutexes within the handler will
    // not deadlock; it merely makes other faults less likely.
    //
    // WARNING:  This routine must be invoked from a signal handler, and the
    // signal handler must either loop forever or terminate the process.
    // Attempts to return from (or `longjmp` out of) the signal handler once this
    // call has been made may cause arbitrary program behaviour including
    // crashes and deadlocks.
    static void internal_attempt_to_use_mutex_in_fatal_signal_handler();

  private:
    std::atomic<intptr_t> mu_;  // The mutex state.

    // post()/wait() versus associated per_thread_sem; in class for required
    // friendship with per_thread_sem.
    static ABEL_FORCE_INLINE void increment_synch_sem(mutex *mu,
                                                      thread_internal::per_thread_synch *w);

    static ABEL_FORCE_INLINE bool decrement_synch_sem(
            mutex *mu, thread_internal::per_thread_synch *w,
            thread_internal::kernel_timeout t);

    // slow path acquire
    void lock_slow_loop(synch_wait_params *waitp, int flags);

    // wrappers around lock_slow_loop()
    bool lock_slow_with_deadline(mu_how how, const condition *cond,
                                 thread_internal::kernel_timeout t,
                                 int flags);

    void lock_slow(mu_how how, const condition *cond,
                   int flags) ABEL_COLD;

    // slow path release
    void unlock_slow(synch_wait_params *waitp) ABEL_COLD;

    // Common code between await() and await_with_timeout/Deadline()
    bool await_common(const condition &cond,
                      thread_internal::kernel_timeout t);

    // Attempt to remove thread s from queue.
    void try_remove(thread_internal::per_thread_synch *s);

    // block a thread on mutex.
    void block(thread_internal::per_thread_synch *s);

    // Wake a thread; return successor.
    thread_internal::per_thread_synch *wakeup(thread_internal::per_thread_synch *w);

    friend class cond_var;   // for access to trans()/fer().
    void trans(mu_how how);  // used for cond_var->mutex transfer
    void fer(thread_internal::per_thread_synch *w);  // used for cond_var->mutex transfer

    // Catch the error of writing mutex when intending mutex_lock.
    mutex(const volatile mutex * /*ignored*/) {}  // NOLINT(runtime/explicit)

    mutex(const mutex &) = delete;

    mutex &operator=(const mutex &) = delete;
};

// -----------------------------------------------------------------------------
// mutex RAII Wrappers
// -----------------------------------------------------------------------------

// mutex_lock
//
// `mutex_lock` is a helper class, which acquires and releases a `mutex` via
// RAII.
//
// Example:
//
// Class Foo {
//
//   Foo::Bar* Baz() {
//     mutex_lock l(&lock_);
//     ...
//     return bar;
//   }
//
// private:
//   mutex lock_;
// };
class ABEL_SCOPED_LOCKABLE mutex_lock {
  public:
    explicit mutex_lock(mutex *mu) ABEL_EXCLUSIVE_LOCK_FUNCTION(mu): mu_(mu) {
        this->mu_->lock();
    }

    mutex_lock(const mutex_lock &) = delete;  // NOLINT(runtime/mutex)
    mutex_lock(mutex_lock &&) = delete;  // NOLINT(runtime/mutex)
    mutex_lock &operator=(const mutex_lock &) = delete;

    mutex_lock &operator=(mutex_lock &&) = delete;

    ~mutex_lock() ABEL_UNLOCK_FUNCTION() { this->mu_->unlock(); }

  private:
    mutex *const mu_;
};

// reader_mutex_lock
//
// The `reader_mutex_lock` is a helper class, like `mutex_lock`, which acquires and
// releases a shared lock on a `mutex` via RAII.
class ABEL_SCOPED_LOCKABLE reader_mutex_lock {
  public:
    explicit reader_mutex_lock(mutex *mu) ABEL_SHARED_LOCK_FUNCTION(mu): mu_(mu) {
        mu->reader_lock();
    }

    reader_mutex_lock(const reader_mutex_lock &) = delete;

    reader_mutex_lock(reader_mutex_lock &&) = delete;

    reader_mutex_lock &operator=(const reader_mutex_lock &) = delete;

    reader_mutex_lock &operator=(reader_mutex_lock &&) = delete;

    ~reader_mutex_lock() ABEL_UNLOCK_FUNCTION() { this->mu_->reader_unlock(); }

  private:
    mutex *const mu_;
};

// writer_mutex_lock
//
// The `writer_mutex_lock` is a helper class, like `mutex_lock`, which acquires and
// releases a write (exclusive) lock on a `mutex` via RAII.
class ABEL_SCOPED_LOCKABLE writer_mutex_lock {
  public:
    explicit writer_mutex_lock(mutex *mu) ABEL_EXCLUSIVE_LOCK_FUNCTION(mu)
            : mu_(mu) {
        mu->writer_lock();
    }

    writer_mutex_lock(const writer_mutex_lock &) = delete;

    writer_mutex_lock(writer_mutex_lock &&) = delete;

    writer_mutex_lock &operator=(const writer_mutex_lock &) = delete;

    writer_mutex_lock &operator=(writer_mutex_lock &&) = delete;

    ~writer_mutex_lock() ABEL_UNLOCK_FUNCTION() { this->mu_->writer_unlock(); }

  private:
    mutex *const mu_;
};

// -----------------------------------------------------------------------------
// condition
// -----------------------------------------------------------------------------
//
// As noted above, `mutex` contains a number of member functions which take a
// `condition` as an argument; clients can wait for conditions to become `true`
// before attempting to acquire the mutex. These sections are known as
// "condition critical" sections. To use a `condition`, you simply need to
// construct it, and use within an appropriate `mutex` member function;
// everything else in the `condition` class is an implementation detail.
//
// A `condition` is specified as a function pointer which returns a boolean.
// `condition` functions should be pure functions -- their results should depend
// only on passed arguments, should not consult any external state (such as
// clocks), and should have no side-effects, aside from debug logging. Any
// objects that the function may access should be limited to those which are
// constant while the mutex is blocked on the condition (e.g. a stack variable),
// or objects of state protected explicitly by the mutex.
//
// No matter which construction is used for `condition`, the underlying
// function pointer / functor / callable must not throw any
// exceptions. Correctness of `mutex` / `condition` is not guaranteed in
// the face of a throwing `condition`. (When abel is allowed to depend
// on C++17, these function pointers will be explicitly marked
// `noexcept`; until then this requirement cannot be enforced in the
// type system.)
//
// Note: to use a `condition`, you need only construct it and pass it within the
// appropriate `mutex' member function, such as `mutex::await()`.
//
// Example:
//
//   // assume count_ is not internal reference count
//   int count_ ABEL_GUARDED_BY(mu_);
//
//   mu_.lock_when(condition(+[](int* count) { return *count == 0; },
//         &count_));
//
// When multiple threads are waiting on exactly the same condition, make sure
// that they are constructed with the same parameters (same pointer to function
// + arg, or same pointer to object + method), so that the mutex implementation
// can avoid redundantly evaluating the same condition for each thread.
class condition {
  public:
    // A condition that returns the result of "(*func)(arg)"
    condition(bool (*func)(void *), void *arg);

    // Templated version for people who are averse to casts.
    //
    // To use a lambda, prepend it with unary plus, which converts the lambda
    // into a function pointer:
    //     condition(+[](T* t) { return ...; }, arg).
    //
    // Note: lambdas in this case must contain no bound variables.
    //
    // See class comment for performance advice.
    template<typename T>
    condition(bool (*func)(T *), T *arg);

    // Templated version for invoking a method that returns a `bool`.
    //
    // `condition(object, &Class::Method)` constructs a `condition` that evaluates
    // `object->Method()`.
    //
    // Implementation Note: `abel::internal::identity` is used to allow methods to
    // come from base classes. A simpler signature like
    // `condition(T*, bool (T::*)())` does not suffice.
    template<typename T>
    condition(T *object, bool (abel::identity<T>::type::* method)());

    // Same as above, for const members
    template<typename T>
    condition(const T *object,
              bool (abel::identity<T>::type::* method)() const);

    // A condition that returns the value of `*cond`
    explicit condition(const bool *cond);

    // Templated version for invoking a functor that returns a `bool`.
    // This approach accepts pointers to non-mutable lambdas, `std::function`,
    // the result of` std::bind` and user-defined functors that define
    // `bool F::operator()() const`.
    //
    // Example:
    //
    //   auto reached = [this, current]() {
    //     mu_.assert_reader_held();                // For annotalysis.
    //     return processed_ >= current;
    //   };
    //   mu_.await(condition(&reached));

    // See class comment for performance advice. In particular, if there
    // might be more than one waiter for the same condition, make sure
    // that all waiters construct the condition with the same pointers.

    // Implementation note: The second template parameter ensures that this
    // constructor doesn't participate in overload resolution if T doesn't have
    // `bool operator() const`.
    template<typename T, typename E = decltype(
    static_cast<bool (T::*)() const>(&T::operator()))>
    explicit condition(const T *obj)
            : condition(obj, static_cast<bool (T::*)() const>(&T::operator())) {}

    // A condition that always returns `true`.
    static const condition kTrue;

    // Evaluates the condition.
    bool eval() const;

    // Returns `true` if the two conditions are guaranteed to return the same
    // value if evaluated at the same time, `false` if the evaluation *may* return
    // different results.
    //
    // Two `condition` values are guaranteed equal if both their `func` and `arg`
    // components are the same. A null pointer is equivalent to a `true`
    // condition.
    static bool guaranteed_equal(const condition *a, const condition *b);

  private:
    typedef bool (*InternalFunctionType)(void *arg);

    typedef bool (condition::*InternalMethodType)();

    typedef bool (*InternalMethodCallerType)(void *arg,
                                             InternalMethodType internal_method);

    bool (*eval_)(const condition *);  // Actual evaluator
    InternalFunctionType function_;   // function taking pointer returning bool
    InternalMethodType method_;       // method returning bool
    void *arg_;                       // arg of function_ or object of method_

    condition();        // null constructor used only to create kTrue

    // Various functions eval_ can point to:
    static bool call_void_ptr_function(const condition *);

    template<typename T>
    static bool CastAndCallFunction(const condition *c);

    template<typename T>
    static bool CastAndCallMethod(const condition *c);
};

// -----------------------------------------------------------------------------
// cond_var
// -----------------------------------------------------------------------------
//
// A condition variable, reflecting state evaluated separately outside of the
// `mutex` object, which can be signaled to wake callers.
// This class is not normally needed; use `mutex` member functions such as
// `mutex::await()` and intrinsic `condition` abstractions. In rare cases
// with many threads and many conditions, `cond_var` may be faster.
//
// The implementation may deliver signals to any condition variable at
// any time, even when no call to `signal()` or `signal_all()` is made; as a
// result, upon being awoken, you must check the logical condition you have
// been waiting upon.
//
// Examples:
//
// Usage for a thread waiting for some condition C protected by mutex mu:
//       mu.lock();
//       while (!C) { cv->wait(&mu); }        // releases and reacquires mu
//       //  C holds; process data
//       mu.unlock();
//
// Usage to wake T is:
//       mu.lock();
//      // process data, possibly establishing C
//      if (C) { cv->signal(); }
//      mu.unlock();
//
// If C may be useful to more than one waiter, use `signal_all()` instead of
// `signal()`.
//
// With this implementation it is efficient to use `signal()/signal_all()` inside
// the locked region; this usage can make reasoning about your program easier.
//
class cond_var {
  public:
    cond_var();

    ~cond_var();

    // cond_var::wait()
    //
    // Atomically releases a `mutex` and blocks on this condition variable.
    // Waits until awakened by a call to `signal()` or `signal_all()` (or a
    // spurious wakeup), then reacquires the `mutex` and returns.
    //
    // Requires and ensures that the current thread holds the `mutex`.
    void wait(mutex *mu);

    // cond_var::wait_with_timeout()
    //
    // Atomically releases a `mutex` and blocks on this condition variable.
    // Waits until awakened by a call to `signal()` or `signal_all()` (or a
    // spurious wakeup), or until the timeout has expired, then reacquires
    // the `mutex` and returns.
    //
    // Returns true if the timeout has expired without this `cond_var`
    // being signalled in any manner. If both the timeout has expired
    // and this `cond_var` has been signalled, the implementation is free
    // to return `true` or `false`.
    //
    // Requires and ensures that the current thread holds the `mutex`.
    bool wait_with_timeout(mutex *mu, abel::duration timeout);

    // cond_var::wait_with_deadline()
    //
    // Atomically releases a `mutex` and blocks on this condition variable.
    // Waits until awakened by a call to `signal()` or `signal_all()` (or a
    // spurious wakeup), or until the deadline has passed, then reacquires
    // the `mutex` and returns.
    //
    // Deadlines in the past are equivalent to an immediate deadline.
    //
    // Returns true if the deadline has passed without this `cond_var`
    // being signalled in any manner. If both the deadline has passed
    // and this `cond_var` has been signalled, the implementation is free
    // to return `true` or `false`.
    //
    // Requires and ensures that the current thread holds the `mutex`.
    bool wait_with_deadline(mutex *mu, abel::time_point deadline);

    // cond_var::signal()
    //
    // signal this `cond_var`; wake at least one waiter if one exists.
    void signal();

    // cond_var::signal_all()
    //
    // signal this `cond_var`; wake all waiters.
    void signal_all();

    // cond_var::enable_debug_log()
    //
    // Causes all subsequent uses of this `cond_var` to be logged via
    // `DLOG_INFO`. Log entries are tagged with `name` if `name != 0`.
    // Note: this method substantially reduces `cond_var` performance.
    void enable_debug_log(const char *name);

  private:

    bool WaitCommon(mutex *mutex, thread_internal::kernel_timeout t);

    void Remove(thread_internal::per_thread_synch *s);

    void wakeup(thread_internal::per_thread_synch *w);

    std::atomic<intptr_t> cv_;  // condition variable state.

    cond_var(const cond_var &) = delete;

    cond_var &operator=(const cond_var &) = delete;
};


// Variants of mutex_lock.
//
// If you find yourself using one of these, consider instead using
// mutex::unlock() and/or if-statements for clarity.

// mutex_lock_maybe
//
// mutex_lock_maybe is like mutex_lock, but is a no-op when mu is null.
class ABEL_SCOPED_LOCKABLE mutex_lock_maybe {
  public:
    explicit mutex_lock_maybe(mutex *mu) ABEL_EXCLUSIVE_LOCK_FUNCTION(mu)
            : mu_(mu) {
        if (this->mu_ != nullptr) {
            this->mu_->lock();
        }
    }

    ~mutex_lock_maybe() ABEL_UNLOCK_FUNCTION() {
        if (this->mu_ != nullptr) { this->mu_->unlock(); }
    }

  private:
    mutex *const mu_;

    mutex_lock_maybe(const mutex_lock_maybe &) = delete;

    mutex_lock_maybe(mutex_lock_maybe &&) = delete;

    mutex_lock_maybe &operator=(const mutex_lock_maybe &) = delete;

    mutex_lock_maybe &operator=(mutex_lock_maybe &&) = delete;
};

// releasable_mutex_lock
//
// releasable_mutex_lock is like mutex_lock, but permits `Release()` of its
// mutex before destruction. `Release()` may be called at most once.
class ABEL_SCOPED_LOCKABLE releasable_mutex_lock {
  public:
    explicit releasable_mutex_lock(mutex *mu) ABEL_EXCLUSIVE_LOCK_FUNCTION(mu)
            : mu_(mu) {
        this->mu_->lock();
    }

    ~releasable_mutex_lock() ABEL_UNLOCK_FUNCTION() {
        if (this->mu_ != nullptr) { this->mu_->unlock(); }
    }

    void Release() ABEL_UNLOCK_FUNCTION();

  private:
    mutex *mu_;

    releasable_mutex_lock(const releasable_mutex_lock &) = delete;

    releasable_mutex_lock(releasable_mutex_lock &&) = delete;

    releasable_mutex_lock &operator=(const releasable_mutex_lock &) = delete;

    releasable_mutex_lock &operator=(releasable_mutex_lock &&) = delete;
};


ABEL_FORCE_INLINE mutex::mutex() : mu_(0) {
    ABEL_TSAN_MUTEX_CREATE(this, __tsan_mutex_not_static);
}

ABEL_FORCE_INLINE constexpr mutex::mutex(abel::const_init_type) : mu_(0) {}

ABEL_FORCE_INLINE cond_var::cond_var() : cv_(0) {}


// static
template<typename T>
bool condition::CastAndCallMethod(const condition *c) {
    typedef bool (T::*MemberType)();
    MemberType rm = reinterpret_cast<MemberType>(c->method_);
    T *x = static_cast<T *>(c->arg_);
    return (x->*rm)();
}

// static
template<typename T>
bool condition::CastAndCallFunction(const condition *c) {
    typedef bool (*FuncType)(T *);
    FuncType fn = reinterpret_cast<FuncType>(c->function_);
    T *x = static_cast<T *>(c->arg_);
    return (*fn)(x);
}

template<typename T>
ABEL_FORCE_INLINE condition::condition(bool (*func)(T *), T *arg)
        : eval_(&CastAndCallFunction<T>),
          function_(reinterpret_cast<InternalFunctionType>(func)),
          method_(nullptr),
          arg_(const_cast<void *>(static_cast<const void *>(arg))) {}

template<typename T>
ABEL_FORCE_INLINE condition::condition(T *object,
                                       bool (abel::identity<T>::type::*method)())
        : eval_(&CastAndCallMethod<T>),
          function_(nullptr),
          method_(reinterpret_cast<InternalMethodType>(method)),
          arg_(object) {}

template<typename T>
ABEL_FORCE_INLINE condition::condition(const T *object,
                                       bool (abel::identity<T>::type::*method)()
                                       const)
        : eval_(&CastAndCallMethod<T>),
          function_(nullptr),
          method_(reinterpret_cast<InternalMethodType>(method)),
          arg_(reinterpret_cast<void *>(const_cast<T *>(object))) {}

// Register a hook for profiling support.
//
// The function pointer registered here will be called whenever a mutex is
// contended.  The callback is given the abel/base/cycleclock.h timestamp when
// waiting began.
//
// Calls to this function do not race or block, but there is no ordering
// guaranteed between calls to this function and call to the provided hook.
// In particular, the previously registered hook may still be called for some
// time after this function returns.
void register_mutex_profiler(void (*fn)(int64_t wait_timestamp));

// Register a hook for mutex tracing.
//
// The function pointer registered here will be called whenever a mutex is
// contended.  The callback is given an opaque handle to the contended mutex,
// an event name, and the number of wait cycles (as measured by
// //abel/base/internal/cycleclock.h, and which may not be real
// "cycle" counts.)
//
// The only event name currently sent is "slow release".
//
// This has the same memory ordering concerns as register_mutex_profiler() above.
void register_mutex_tracer(void (*fn)(const char *msg, const void *obj,
                                      int64_t wait_cycles));

// TODO(gfalcon): Combine register_mutex_profiler() and register_mutex_tracer()
// into a single interface, since they are only ever called in pairs.

// Register a hook for cond_var tracing.
//
// The function pointer registered here will be called here on various cond_var
// events.  The callback is given an opaque handle to the cond_var object and
// a string identifying the event.  This is thread-safe, but only a single
// tracer can be registered.
//
// Events that can be sent are "wait", "Unwait", "signal wakeup", and
// "signal_all wakeup".
//
// This has the same memory ordering concerns as register_mutex_profiler() above.
void register_cond_var_tracer(void (*fn)(const char *msg, const void *cv));

// Register a hook for symbolizing stack traces in deadlock detector reports.
//
// 'pc' is the program counter being symbolized, 'out' is the buffer to write
// into, and 'out_size' is the size of the buffer.  This function can return
// false if symbolizing failed, or true if a NUL-terminated symbol was written
// to 'out.'
//
// This has the same memory ordering concerns as register_mutex_profiler() above.
//
// DEPRECATED: The default symbolizer function is abel::symbolize() and the
// ability to register a different hook for symbolizing stack traces will be
// removed on or after 2023-05-01.
ABEL_DEPRECATED_MESSAGE("abel::register_symbolizer() is deprecated and will be removed "
                        "on or after 2023-05-01")
void register_symbolizer(bool (*fn)(const void *pc, char *out, int out_size));

// enable_mutex_invariant_debugging()
//
// Enable or disable global support for mutex invariant debugging.  If enabled,
// then invariant predicates can be registered per-mutex for debug checking.
// See mutex::enable_invariant_debugging().
void enable_mutex_invariant_debugging(bool enabled);

// When in debug mode, and when the feature has been enabled globally, the
// implementation will keep track of lock ordering and complain (or optionally
// crash) if a cycle is detected in the acquired-before graph.

// Possible modes of operation for the deadlock detector in debug mode.
enum class on_deadlock_cycle {
    kIgnore,  // Neither report on nor attempt to track cycles in lock ordering
    kReport,  // Report lock cycles to stderr when detected
    kAbort,  // Report lock cycles to stderr when detected, then abort
};

// set_mutex_deadlock_detection_mode()
//
// Enable or disable global support for detection of potential deadlocks
// due to mutex lock ordering inversions.  When set to 'kIgnore', tracking of
// lock ordering is disabled.  Otherwise, in debug builds, a lock ordering graph
// will be maintained internally, and detected cycles will be reported in
// the manner chosen here.
void set_mutex_deadlock_detection_mode(on_deadlock_cycle mode);


}  // namespace abel

// In some build configurations we pass --detect-odr-violations to the
// gold linker.  This causes it to flag weak symbol overrides as ODR
// violations.  Because ODR only applies to C++ and not C,
// --detect-odr-violations ignores symbols not mangled with C++ names.
// By changing our extension points to be extern "C", we dodge this
// check.
extern "C" {
void abel_internal_mutex_yield();
}  // extern "C"

#endif  // ABEL_SYNCHRONIZATION_MUTEX_H_
