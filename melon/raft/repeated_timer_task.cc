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


#include <melon/raft/repeated_timer_task.h>
#include <melon/raft/util.h>

namespace melon::raft {

    RepeatedTimerTask::RepeatedTimerTask()
            : _timeout_ms(0), _stopped(true), _running(false), _destroyed(true), _invoking(false) {}

    RepeatedTimerTask::~RepeatedTimerTask() {
        MCHECK(!_running) << "Is still running";
        MCHECK(_destroyed) << "destroy() must be invoked before descrution";
    }

    int RepeatedTimerTask::init(int timeout_ms) {
        _timeout_ms = timeout_ms;
        _destroyed = false;
        _stopped = true;
        _running = false;
        _timer = fiber_timer_t();
        return 0;
    }

    void RepeatedTimerTask::stop() {
        MELON_SCOPED_LOCK(_mutex);
        BRAFT_RETURN_IF(_stopped);
        _stopped = true;
        MCHECK(_running);
        const int rc = fiber_timer_del(_timer);
        if (rc == 0) {
            _running = false;
            return;
        }
    }

    void RepeatedTimerTask::on_timedout() {
        std::unique_lock<raft_mutex_t> lck(_mutex);
        _invoking = true;
        lck.unlock();
        //   ^^^NOTE: don't invoke run() inside lock to avoid the dead-lock issue
        run();
        lck.lock();
        _invoking = false;
        MCHECK(_running);
        if (_stopped) {
            _running = false;
            if (_destroyed) {
                // this may call the destruction,
                // so do this after setting _running to false
                lck.unlock();
                on_destroy();
            }
            return;
        }
        return schedule(lck);
    }

    void RepeatedTimerTask::start() {
        // Implementation considers the following conditions:
        //   - The first time start() was invoked
        //   - stop() was not invoked()
        //   - stop() was invoked and _timer was successfully deleted
        //   - stop() was invoked but _timer was not successfully deleted:
        //       a) _timer is still running right now
        //       b) _timer is finished
        std::unique_lock<raft_mutex_t> lck(_mutex);
        BRAFT_RETURN_IF(_destroyed);
        BRAFT_RETURN_IF(!_stopped);
        _stopped = false;

        BRAFT_RETURN_IF(_running);
        //  ^^^ _timer was not successfully deleted and the former task
        // is still running, in which case on_timedout would invoke
        // schedule as it would not see _stopped
        _running = true;
        schedule(lck);
    }

    void RepeatedTimerTask::run_once_now() {
        std::unique_lock<raft_mutex_t> lck(_mutex);
        if (fiber_timer_del(_timer) == 0) {
            lck.unlock();
            on_timedout(this);
        }
    }

    void *RepeatedTimerTask::run_on_timedout_in_new_thread(void *arg) {
        RepeatedTimerTask *m = (RepeatedTimerTask *) arg;
        m->on_timedout();
        return NULL;
    }

    void RepeatedTimerTask::on_timedout(void *arg) {
        // Start a fiber to invoke run() so we won't block the timer thread.
        // as run() might access the disk so the time it takes is probably beyond
        // expection
        fiber_t tid;
        if (fiber_start_background(
                &tid, NULL, run_on_timedout_in_new_thread, arg) != 0) {
            PMLOG(ERROR) << "Fail to start fiber";
            run_on_timedout_in_new_thread(arg);
        }
    }

    void RepeatedTimerTask::schedule(std::unique_lock<raft_mutex_t> &lck) {
        _next_duetime =
                mutil::milliseconds_from_now(adjust_timeout_ms(_timeout_ms));
        if (fiber_timer_add(&_timer, _next_duetime, on_timedout, this) != 0) {
            lck.unlock();
            MLOG(ERROR) << "Fail to add timer";
            return on_timedout(this);
        }
    }

    void RepeatedTimerTask::reset() {
        std::unique_lock<raft_mutex_t> lck(_mutex);
        BRAFT_RETURN_IF(_stopped);
        MCHECK(_running);
        const int rc = fiber_timer_del(_timer);
        if (rc == 0) {
            return schedule(lck);
        }
        // else on_timedout would invoke schdule
    }

    void RepeatedTimerTask::reset(int timeout_ms) {
        std::unique_lock<raft_mutex_t> lck(_mutex);
        _timeout_ms = timeout_ms;
        BRAFT_RETURN_IF(_stopped);
        MCHECK(_running);
        const int rc = fiber_timer_del(_timer);
        if (rc == 0) {
            return schedule(lck);
        }
        // else on_timedout would invoke schdule
    }

    void RepeatedTimerTask::destroy() {
        std::unique_lock<raft_mutex_t> lck(_mutex);
        BRAFT_RETURN_IF(_destroyed);
        _destroyed = true;
        if (!_running) {
            MCHECK(_stopped);
            lck.unlock();
            on_destroy();
            return;
        }
        BRAFT_RETURN_IF(_stopped);
        _stopped = true;
        const int rc = fiber_timer_del(_timer);
        if (rc == 0) {
            _running = false;
            lck.unlock();
            on_destroy();
            return;
        }
        MCHECK(_running);
    }

    void RepeatedTimerTask::describe(std::ostream &os, bool use_html) {
        (void) use_html;
        std::unique_lock<raft_mutex_t> lck(_mutex);
        const bool stopped = _stopped;
        const bool running = _running;
        const bool destroyed = _destroyed;
        const bool invoking = _invoking;
        const timespec duetime = _next_duetime;
        const int timeout_ms = _timeout_ms;
        lck.unlock();
        os << "timeout(" << timeout_ms << "ms)";
        if (destroyed) {
            os << " DESTROYED";
        }
        if (stopped) {
            os << " STOPPED";
        }
        if (running) {
            if (invoking) {
                os << " INVOKING";
            } else {
                os << " SCHEDULING(in "
                   << mutil::timespec_to_milliseconds(duetime) - mutil::gettimeofday_ms()
                   << "ms)";
            }
        }
    }

}  //  namespace melon::raft

