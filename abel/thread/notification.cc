// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/thread/notification.h"
#include "abel/base/profile.h"
#include "abel/log/logging.h"
#include "abel/thread/mutex.h"
#include "abel/chrono/time.h"

#include <atomic>

namespace abel {


void notification::notify() {
    mutex_lock l(&this->mutex_);

#ifndef NDEBUG
    if (ABEL_UNLIKELY(notified_yet_.load(std::memory_order_relaxed))) {
        DLOG_CRITICAL("notify() method called more than once for notification object {:p}",
                      static_cast<void *>(this));
    }
#endif

    notified_yet_.store(true, std::memory_order_release);
}

notification::~notification() {
    // Make sure that the thread running notify() exits before the object is
    // destructed.
    mutex_lock l(&this->mutex_);
}

void notification::wait_for_notification() const {
    if (!has_been_notified_internal(&this->notified_yet_)) {
        this->mutex_.lock_when(condition(&has_been_notified_internal,
                                         &this->notified_yet_));
        this->mutex_.unlock();
    }
}

bool notification::wait_for_notification_with_timeout(
        abel::duration timeout) const {
    bool notified = has_been_notified_internal(&this->notified_yet_);
    if (!notified) {
        notified = this->mutex_.lock_when_with_timeout(
                condition(&has_been_notified_internal, &this->notified_yet_), timeout);
        this->mutex_.unlock();
    }
    return notified;
}

bool notification::wait_for_notification_with_deadline(abel::abel_time deadline) const {
    bool notified = has_been_notified_internal(&this->notified_yet_);
    if (!notified) {
        notified = this->mutex_.lock_when_with_deadline(
                condition(&has_been_notified_internal, &this->notified_yet_), deadline);
        this->mutex_.unlock();
    }
    return notified;
}


}  // namespace abel
