//

#include <abel/thread/barrier.h>

#include <abel/log/raw_logging.h>
#include <abel/thread/mutex.h>

namespace abel {


// Return whether int *arg is zero.
    static bool IsZero(void *arg) {
        return 0 == *reinterpret_cast<int *>(arg);
    }

    bool barrier::block() {
        mutex_lock l(&this->lock_);

        this->num_to_block_--;
        if (this->num_to_block_ < 0) {
            ABEL_RAW_LOG(
                    FATAL,
                    "Block() called too many times.  num_to_block_=%d out of total=%d",
                    this->num_to_block_, this->num_to_exit_);
        }

        this->lock_.Await(condition(IsZero, &this->num_to_block_));

        // Determine which thread can safely delete this barrier object
        this->num_to_exit_--;
        ABEL_RAW_CHECK(this->num_to_exit_ >= 0, "barrier underflow");

        // If num_to_exit_ == 0 then all other threads in the barrier have
        // exited the wait() and have released the mutex so this thread is
        // free to delete the barrier.
        return this->num_to_exit_ == 0;
    }


}  // namespace abel
