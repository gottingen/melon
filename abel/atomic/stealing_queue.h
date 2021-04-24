// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_ATOMIC_STEALING_QUEUE_H_
#define ABEL_ATOMIC_STEALING_QUEUE_H_

#include "abel/log/logging.h"
#include "abel/base/profile.h"
#include <atomic>

namespace abel {


template<typename T>
class stealing_queue {
  public:
    stealing_queue()
            : _bottom(1), _capacity(0), _buffer(NULL), _top(1) {
    }

    ~stealing_queue() {
        delete[] _buffer;
        _buffer = NULL;
    }

    int init(size_t capacity) {
        if (_capacity != 0) {
            DLOG_ERROR("Already initialized");
            return -1;
        }
        if (capacity == 0) {
            DLOG_ERROR("Invalid capacity={}", capacity);
            return -1;
        }
        if (capacity & (capacity - 1)) {
            DLOG_ERROR("Invalid capacity={} which must be power of 2", capacity);
            return -1;
        }
        _buffer = new(std::nothrow) T[capacity];
        if (NULL == _buffer) {
            return -1;
        }
        _capacity = capacity;
        return 0;
    }

    // Push an item into the queue.
    // Returns true on pushed.
    // May run in parallel with steal().
    // Never run in parallel with pop() or another push().
    bool push(const T &x) {
        const size_t b = _bottom.load(std::memory_order_relaxed);
        const size_t t = _top.load(std::memory_order_acquire);
        if (b >= t + _capacity) { // Full queue.
            return false;
        }
        _buffer[b & (_capacity - 1)] = x;
        _bottom.store(b + 1, std::memory_order_release);
        return true;
    }

    // Pop an item from the queue.
    // Returns true on popped and the item is written to `val'.
    // May run in parallel with steal().
    // Never run in parallel with push() or another pop().
    bool pop(T *val) {
        const size_t b = _bottom.load(std::memory_order_relaxed);
        size_t t = _top.load(std::memory_order_relaxed);
        if (t >= b) {
            // fast check since we call pop() in each sched.
            // Stale _top which is smaller should not enter this branch.
            return false;
        }
        const size_t newb = b - 1;
        _bottom.store(newb, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        t = _top.load(std::memory_order_relaxed);
        if (t > newb) {
            _bottom.store(b, std::memory_order_relaxed);
            return false;
        }
        *val = _buffer[newb & (_capacity - 1)];
        if (t != newb) {
            return true;
        }
        // Single last element, compete with steal()
        const bool popped = _top.compare_exchange_strong(
                t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed);
        _bottom.store(b, std::memory_order_relaxed);
        return popped;
    }

    // Steal one item from the queue.
    // Returns true on stolen.
    // May run in parallel with push() pop() or another steal().
    bool steal(T *val) {
        size_t t = _top.load(std::memory_order_acquire);
        size_t b = _bottom.load(std::memory_order_acquire);
        if (t >= b) {
            // Permit false negative for performance considerations.
            return false;
        }
        do {
            std::atomic_thread_fence(std::memory_order_seq_cst);
            b = _bottom.load(std::memory_order_acquire);
            if (t >= b) {
                return false;
            }
            *val = _buffer[t & (_capacity - 1)];
        } while (!_top.compare_exchange_strong(t, t + 1,
                                               std::memory_order_seq_cst,
                                               std::memory_order_relaxed));
        return true;
    }

    size_t volatile_size() const {
        const size_t b = _bottom.load(std::memory_order_relaxed);
        const size_t t = _top.load(std::memory_order_relaxed);
        return (b <= t ? 0 : (b - t));
    }

    size_t capacity() const { return _capacity; }

  private:
    // Copying a concurrent structure makes no sense.
    ABEL_NON_COPYABLE(stealing_queue);

    std::atomic<size_t> _bottom;
    size_t _capacity;
    T *_buffer;
    std::atomic<size_t> ABEL_CACHE_LINE_ALIGNED _top;
};


}  // namespace abel

#endif  // ABEL_ATOMIC_STEALING_QUEUE_H_
