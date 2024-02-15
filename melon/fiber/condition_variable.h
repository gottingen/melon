// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#ifndef  MELON_FIBER_CONDITION_VARIABLE_H_
#define  MELON_FIBER_CONDITION_VARIABLE_H_

#include "melon/utility/time.h"
#include "melon/fiber/mutex.h"

__BEGIN_DECLS
extern int fiber_cond_init(fiber_cond_t* __restrict cond,
                             const fiber_condattr_t* __restrict cond_attr);
extern int fiber_cond_destroy(fiber_cond_t* cond);
extern int fiber_cond_signal(fiber_cond_t* cond);
extern int fiber_cond_broadcast(fiber_cond_t* cond);
extern int fiber_cond_wait(fiber_cond_t* __restrict cond,
                             fiber_mutex_t* __restrict mutex);
extern int fiber_cond_timedwait(
    fiber_cond_t* __restrict cond,
    fiber_mutex_t* __restrict mutex,
    const struct timespec* __restrict abstime);
__END_DECLS

namespace fiber {

class ConditionVariable {
    DISALLOW_COPY_AND_ASSIGN(ConditionVariable);
public:
    typedef fiber_cond_t*         native_handler_type;
    
    ConditionVariable() {
        CHECK_EQ(0, fiber_cond_init(&_cond, NULL));
    }
    ~ConditionVariable() {
        CHECK_EQ(0, fiber_cond_destroy(&_cond));
    }

    native_handler_type native_handler() { return &_cond; }

    void wait(std::unique_lock<fiber::Mutex>& lock) {
        fiber_cond_wait(&_cond, lock.mutex()->native_handler());
    }

    void wait(std::unique_lock<fiber_mutex_t>& lock) {
        fiber_cond_wait(&_cond, lock.mutex());
    }

    // Unlike std::condition_variable, we return ETIMEDOUT when time expires
    // rather than std::timeout
    int wait_for(std::unique_lock<fiber::Mutex>& lock,
                 long timeout_us) {
        return wait_until(lock, mutil::microseconds_from_now(timeout_us));
    }

    int wait_for(std::unique_lock<fiber_mutex_t>& lock,
                 long timeout_us) {
        return wait_until(lock, mutil::microseconds_from_now(timeout_us));
    }

    int wait_until(std::unique_lock<fiber::Mutex>& lock,
                   timespec duetime) {
        const int rc = fiber_cond_timedwait(
                &_cond, lock.mutex()->native_handler(), &duetime);
        return rc == ETIMEDOUT ? ETIMEDOUT : 0;
    }

    int wait_until(std::unique_lock<fiber_mutex_t>& lock,
                   timespec duetime) {
        const int rc = fiber_cond_timedwait(
                &_cond, lock.mutex(), &duetime);
        return rc == ETIMEDOUT ? ETIMEDOUT : 0;
    }

    void notify_one() {
        fiber_cond_signal(&_cond);
    }

    void notify_all() {
        fiber_cond_broadcast(&_cond);
    }

private:
    fiber_cond_t                  _cond;
};

}  // namespace fiber

#endif  // MELON_FIBER_CONDITION_VARIABLE_H_
