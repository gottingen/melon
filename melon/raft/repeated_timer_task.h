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

#pragma once

#include <melon/fiber/unstable.h>
#include <melon/raft/macros.h>

namespace melon::raft {

    // Repeated scheduled timer task
    class RepeatedTimerTask {
        DISALLOW_COPY_AND_ASSIGN(RepeatedTimerTask);

    public:
        RepeatedTimerTask();

        virtual ~RepeatedTimerTask();

        // Initialize timer task
        int init(int timeout_ms);

        // Start the timer
        void start();

        // Run timer function once now
        void run_once_now();

        // Stop the timer
        void stop();

        // Reset the timer, and schedule it in the initial timeout_ms
        void reset();

        // Reset the timer and schedule it in |timeout_ms|
        void reset(int timeout_ms);

        // Destroy the timer
        void destroy();

        // Describe the current status of timer
        void describe(std::ostream &os, bool use_html);

    protected:

        // Invoked everytime when it reaches the timeout
        virtual void run() = 0;

        // Invoked when the timer is finally destroyed
        virtual void on_destroy() = 0;

        virtual int adjust_timeout_ms(int timeout_ms) {
            return timeout_ms;
        }

    private:

        static void on_timedout(void *arg);

        static void *run_on_timedout_in_new_thread(void *arg);

        void on_timedout();

        void schedule(std::unique_lock<raft_mutex_t> &lck);

        raft_mutex_t _mutex;
        fiber_timer_t _timer;
        timespec _next_duetime;
        int _timeout_ms;
        bool _stopped;
        bool _running;
        bool _destroyed;
        bool _invoking;
    };

}  //  namespace melon::raft
