//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#ifndef  MELON_FIBER_CONDITION_VARIABLE_H_
#define  MELON_FIBER_CONDITION_VARIABLE_H_

#include <melon/utility/time.h>
#include <melon/fiber/mutex.h>

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
        MCHECK_EQ(0, fiber_cond_init(&_cond, NULL));
    }
    ~ConditionVariable() {
        MCHECK_EQ(0, fiber_cond_destroy(&_cond));
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
