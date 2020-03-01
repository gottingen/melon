//
//
// -----------------------------------------------------------------------------
// blocking_counter.h
// -----------------------------------------------------------------------------

#ifndef ABEL_SYNCHRONIZATION_BLOCKING_COUNTER_H_
#define ABEL_SYNCHRONIZATION_BLOCKING_COUNTER_H_

#include <abel/threading/thread_annotations.h>
#include <abel/synchronization/mutex.h>

namespace abel {


// blocking_counter
//
// This class allows a thread to block for a pre-specified number of actions.
// `blocking_counter` maintains a single non-negative abstract integer "count"
// with an initial value `initial_count`. A thread can then call `wait()` on
// this blocking counter to block until the specified number of events occur;
// worker threads then call 'decrement_count()` on the counter upon completion of
// their work. Once the counter's internal "count" reaches zero, the blocked
// thread unblocks.
//
// A `blocking_counter` requires the following:
//     - its `initial_count` is non-negative.
//     - the number of calls to `decrement_count()` on it is at most
//       `initial_count`.
//     - `wait()` is called at most once on it.
//
// Given the above requirements, a `blocking_counter` provides the following
// guarantees:
//     - Once its internal "count" reaches zero, no legal action on the object
//       can further change the value of "count".
//     - When `wait()` returns, it is legal to destroy the `blocking_counter`.
//     - When `wait()` returns, the number of calls to `decrement_count()` on
//       this blocking counter exactly equals `initial_count`.
//
// Example:
//     blocking_counter bcount(N);         // there are N items of work
//     ... Allow worker threads to start.
//     ... On completing each work item, workers do:
//     ... bcount.decrement_count();      // an item of work has been completed
//
//     bcount.wait();                    // wait for all work to be complete
//
    class blocking_counter {
    public:
        explicit blocking_counter(int initial_count)
                : count_(initial_count), num_waiting_(0) {}

        blocking_counter(const blocking_counter &) = delete;

        blocking_counter &operator=(const blocking_counter &) = delete;

        // blocking_counter::decrement_count()
        //
        // Decrements the counter's "count" by one, and return "count == 0". This
        // function requires that "count != 0" when it is called.
        //
        // Memory ordering: For any threads X and Y, any action taken by X
        // before it calls `decrement_count()` is visible to thread Y after
        // Y's call to `decrement_count()`, provided Y's call returns `true`.
        bool decrement_count();

        // blocking_counter::wait()
        //
        // Blocks until the counter reaches zero. This function may be called at most
        // once. On return, `decrement_count()` will have been called "initial_count"
        // times and the blocking counter may be destroyed.
        //
        // Memory ordering: For any threads X and Y, any action taken by X
        // before X calls `decrement_count()` is visible to Y after Y returns
        // from `wait()`.
        void wait();

    private:
        mutex lock_;
        int count_ ABEL_GUARDED_BY(lock_);
        int num_waiting_ ABEL_GUARDED_BY(lock_);
    };


}  // namespace abel

#endif  // ABEL_SYNCHRONIZATION_BLOCKING_COUNTER_H_
