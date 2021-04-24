// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "abel/thread/barrier.h"

#include "abel/log/logging.h"
#include "abel/thread/mutex.h"

namespace abel {


// Return whether int *arg is zero.
static bool IsZero(void *arg) {
    return 0 == *reinterpret_cast<int *>(arg);
}

bool barrier::block() {
    mutex_lock l(&this->lock_);

    this->num_to_block_--;
    if (this->num_to_block_ < 0) {
        DLOG_CRITICAL(
                "block() called too many times.  num_to_block_={} out of total={}",
                this->num_to_block_, this->num_to_exit_);
    }

    this->lock_.await(condition(IsZero, &this->num_to_block_));

    // Determine which thread can safely delete this barrier object
    this->num_to_exit_--;
    DCHECK_MSG(this->num_to_exit_ >= 0, "barrier underflow");

    // If num_to_exit_ == 0 then all other threads in the barrier have
    // exited the wait() and have released the mutex so this thread is
    // free to delete the barrier.
    return this->num_to_exit_ == 0;
}


}  // namespace abel
