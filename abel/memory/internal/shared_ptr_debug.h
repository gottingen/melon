// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_MEMORY_INTERNAL_SHARED_PTR_DEBUG_H_
#define ABEL_MEMORY_INTERNAL_SHARED_PTR_DEBUG_H_

#include <thread>
#include <cassert>

namespace abel {

// A counter that is only comfortable being incremented on the cpu
// it was created on.  Useful for verifying that a shared_ptr
// or lw_shared_ptr isn't misued across cores.
class debug_shared_ptr_counter_type {
    long _counter = 0;
    std::thread::id _cpu = std::this_thread::get_id();
  public:
    debug_shared_ptr_counter_type(long x) : _counter(x) {}

    operator long() const {
        check();
        return _counter;
    }

    debug_shared_ptr_counter_type &operator++() {
        check();
        ++_counter;
        return *this;
    }

    long operator++(int) {
        check();
        return _counter++;
    }

    debug_shared_ptr_counter_type &operator--() {
        check();
        --_counter;
        return *this;
    }

    long operator--(int) {
        check();
        return _counter--;
    }

  private:
    void check() const {
        assert(_cpu == std::this_thread::get_id());
    }
};

}  // namespace abel

#endif  // ABEL_MEMORY_INTERNAL_SHARED_PTR_DEBUG_H_
