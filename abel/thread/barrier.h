//
// -----------------------------------------------------------------------------
// barrier.h
// -----------------------------------------------------------------------------

#ifndef ABEL_SYNCHRONIZATION_BARRIER_H_
#define ABEL_SYNCHRONIZATION_BARRIER_H_

#include <abel/thread/thread_annotations.h>
#include <abel/thread/mutex.h>

namespace abel {


// barrier
//
// This class creates a barrier which blocks threads until a prespecified
// threshold of threads (`num_threads`) utilizes the barrier. A thread utilizes
// the `barrier` by calling `Block()` on the barrier, which will block that
// thread; no call to `Block()` will return until `num_threads` threads have
// called it.
//
// Exactly one call to `Block()` will return `true`, which is then responsible
// for destroying the barrier; because stack allocation will cause the barrier
// to be deleted when it is out of scope, barriers should not be stack
// allocated.
//
// Example:
//
//   // Main thread creates a `barrier`:
//   barrier = new barrier(num_threads);
//
//   // Each participating thread could then call:
//   if (barrier->Block()) delete barrier;  // Exactly one call to `Block()`
//                                          // returns `true`; that call
//                                          // deletes the barrier.
    class barrier {
    public:
        // `num_threads` is the number of threads that will participate in the barrier
        explicit barrier(int num_threads)
                : num_to_block_(num_threads), num_to_exit_(num_threads) {}

        barrier(const barrier &) = delete;

        barrier &operator=(const barrier &) = delete;

        // barrier::Block()
        //
        // Blocks the current thread, and returns only when the `num_threads`
        // threshold of threads utilizing this barrier has been reached. `Block()`
        // returns `true` for precisely one caller, which may then destroy the
        // barrier.
        //
        // Memory ordering: For any threads X and Y, any action taken by X
        // before X calls `Block()` will be visible to Y after Y returns from
        // `Block()`.
        bool block();

    private:
        mutex lock_;
        int num_to_block_ ABEL_GUARDED_BY(lock_);
        int num_to_exit_ ABEL_GUARDED_BY(lock_);
    };


}  // namespace abel
#endif  // ABEL_SYNCHRONIZATION_BARRIER_H_
