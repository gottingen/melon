//
// -----------------------------------------------------------------------------
// notification.h
// -----------------------------------------------------------------------------
//
// This header file defines a `notification` abstraction, which allows threads
// to receive notification of a single occurrence of a single event.
//
// The `notification` object maintains a private boolean "notified" state that
// transitions to `true` at most once. The `notification` class provides the
// following primary member functions:
//   * `has_been_notified() `to query its state
//   * `wait_for_notification*()` to have threads wait until the "notified" state
//      is `true`.
//   * `Notify()` to set the notification's "notified" state to `true` and
//     notify all waiting threads that the event has occurred.
//     This method may only be called once.
//
// Note that while `Notify()` may only be called once, it is perfectly valid to
// call any of the `wait_for_notification*()` methods multiple times, from
// multiple threads -- even after the notification's "notified" state has been
// set -- in which case those methods will immediately return.
//
// Note that the lifetime of a `notification` requires careful consideration;
// it might not be safe to destroy a notification after calling `Notify()` since
// it is still legal for other threads to call `wait_for_notification*()` methods
// on the notification. However, observers responding to a "notified" state of
// `true` can safely delete the notification without interfering with the call
// to `Notify()` in the other thread.
//
// Memory ordering: For any threads X and Y, if X calls `Notify()`, then any
// action taken by X before it calls `Notify()` is visible to thread Y after:
//  * Y returns from `wait_for_notification()`, or
//  * Y receives a `true` return value from either `has_been_notified()` or
//    `wait_for_notification_with_timeout()`.

#ifndef ABEL_SYNCHRONIZATION_NOTIFICATION_H_
#define ABEL_SYNCHRONIZATION_NOTIFICATION_H_

#include <atomic>

#include <abel/base/profile.h>
#include <abel/synchronization/mutex.h>
#include <abel/chrono/time.h>

namespace abel {


// -----------------------------------------------------------------------------
// notification
// -----------------------------------------------------------------------------
class notification {
 public:
  // Initializes the "notified" state to unnotified.
  notification() : notified_yet_(false) {}
  explicit notification(bool prenotify) : notified_yet_(prenotify) {}
  notification(const notification&) = delete;
  notification& operator=(const notification&) = delete;
  ~notification();

  // notification::has_been_notified()
  //
  // Returns the value of the notification's internal "notified" state.
  bool has_been_notified() const {
    return has_been_notified_internal(&this->notified_yet_);
  }

  // notification::wait_for_notification()
  //
  // Blocks the calling thread until the notification's "notified" state is
  // `true`. Note that if `Notify()` has been previously called on this
  // notification, this function will immediately return.
  void wait_for_notification() const;

  // notification::wait_for_notification_with_timeout()
  //
  // Blocks until either the notification's "notified" state is `true` (which
  // may occur immediately) or the timeout has elapsed, returning the value of
  // its "notified" state in either case.
  bool wait_for_notification_with_timeout(abel::duration timeout) const;

  // notification::wait_for_notification_with_deadline()
  //
  // Blocks until either the notification's "notified" state is `true` (which
  // may occur immediately) or the deadline has expired, returning the value of
  // its "notified" state in either case.
  bool wait_for_notification_with_deadline(abel::abel_time deadline) const;

  // notification::Notify()
  //
  // Sets the "notified" state of this notification to `true` and wakes waiting
  // threads. Note: do not call `Notify()` multiple times on the same
  // `notification`; calling `Notify()` more than once on the same notification
  // results in undefined behavior.
  void Notify();

 private:
  static ABEL_FORCE_INLINE bool has_been_notified_internal(
      const std::atomic<bool>* notified_yet) {
    return notified_yet->load(std::memory_order_acquire);
  }

  mutable mutex mutex_;
  std::atomic<bool> notified_yet_;  // written under mutex_
};


}  // namespace abel

#endif  // ABEL_SYNCHRONIZATION_NOTIFICATION_H_
