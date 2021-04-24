// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/thread/blocking_counter.h"
#include "abel/log/logging.h"

namespace abel {


// Return whether int *arg is zero.
static bool IsZero(void *arg) {
    return 0 == *reinterpret_cast<int *>(arg);
}

bool blocking_counter::decrement_count() {
    mutex_lock l(&lock_);
    count_--;
    if (count_ < 0) {
        DLOG_CRITICAL(
                "blocking_counter::decrement_count() called too many times.  count={}",
                count_);
    }
    return count_ == 0;
}

void blocking_counter::wait() {
    mutex_lock l(&this->lock_);
    DCHECK_MSG(count_ >= 0, "blocking_counter underflow");

    // only one thread may call wait(). To support more than one thread,
    // implement a counter num_to_exit, like in the barrier class.
    DCHECK_MSG(num_waiting_ == 0, "multiple threads called wait()");
    num_waiting_++;

    this->lock_.await(condition(IsZero, &this->count_));

    // At this point, We know that all threads executing decrement_count have
    // released the lock, and so will not touch this object again.
    // Therefore, the thread calling this method is free to delete the object
    // after we return from this method.
}


}  // namespace abel
