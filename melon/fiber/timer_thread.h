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


#ifndef MELON_FIBER_TIMER_THREAD_H_
#define MELON_FIBER_TIMER_THREAD_H_

#include <vector>                     // std::vector
#include <pthread.h>                  // pthread_*
#include <melon/utility/atomicops.h>
#include <melon/utility/time.h>                // time utilities
#include <melon/fiber/mutex.h>

namespace fiber {

struct TimerThreadOptions {
    // Scheduling requests are hashed into different bucket to improve
    // scalability. However bigger num_buckets may NOT result in more scalable 
    // schedule() because bigger values also make each buckets more sparse
    // and more likely to lock the global mutex. You better not change
    // this value, just leave it to us.
    // Default: 13
    size_t num_buckets;

    // If this field is not empty, some var for reporting stats of TimerThread
    // will be exposed with this prefix.
    // Default: ""
    std::string var_prefix;

    // Constructed with default options.
    TimerThreadOptions();
};

// TimerThread is a separate thread to run scheduled tasks at specific time.
// At most one task runs at any time, don't put time-consuming code in the
// callback otherwise the task may delay other tasks significantly.
class TimerThread {
public:
    struct Task;
    class Bucket;

    typedef uint64_t TaskId;
    const static TaskId INVALID_TASK_ID;

    TimerThread();
    ~TimerThread();

    // Start the timer thread.
    // This method should only be called once.
    // return 0 if success, errno otherwise.
    int start(const TimerThreadOptions* options);

    // Stop the timer thread. Later schedule() will return INVALID_TASK_ID.
    void stop_and_join();

    // Schedule |fn(arg)| to run at realtime |abstime| approximately.
    // Returns: identifier of the scheduled task, INVALID_TASK_ID on error.
    TaskId schedule(void (*fn)(void*), void* arg, const timespec& abstime);

    // Prevent the task denoted by `task_id' from running. `task_id' must be
    // returned by schedule() ever.
    // Returns:
    //   0   -  Removed the task which does not run yet
    //  -1   -  The task does not exist.
    //   1   -  The task is just running.
    int unschedule(TaskId task_id);

    // Get identifier of internal pthread.
    // Returns (pthread_t)0 if start() is not called yet.
    pthread_t thread_id() const { return _thread; }
    
private:
    // the timer thread will run this method.
    void run();
    static void* run_this(void* arg);

    bool _started;            // whether the timer thread was started successfully.
    mutil::atomic<bool> _stop;

    TimerThreadOptions _options;
    Bucket* _buckets;        // list of tasks to be run
    internal::FastPthreadMutex _mutex;    // protect _nearest_run_time
    int64_t _nearest_run_time;
    // the futex for wake up timer thread. can't use _nearest_run_time because
    // it's 64-bit.
    int _nsignals;
    pthread_t _thread;       // all scheduled task will be run on this thread
};

// Get the global TimerThread which never quits.
TimerThread* get_or_create_global_timer_thread();
TimerThread* get_global_timer_thread();

}   // end namespace fiber

#endif  // MELON_FIBER_TIMER_THREAD_H_
