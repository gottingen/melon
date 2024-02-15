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


#ifndef MELON_FIBER_COUNTDOWN_EVENT_H_
#define MELON_FIBER_COUNTDOWN_EVENT_H_

#include "melon/fiber/fiber.h"

namespace fiber {

// A synchronization primitive to wait for multiple signallers.
class CountdownEvent {
public:
    CountdownEvent(int initial_count = 1);
    ~CountdownEvent();

    // Increase current counter by |v|
    void add_count(int v = 1);

    // Reset the counter to |v|
    void reset(int v = 1);

    // Decrease the counter by |sig|
    // when flush is true, after signal we need to call fiber_flush
    void signal(int sig = 1, bool flush = false);

    // Block current thread until the counter reaches 0.
    // Returns 0 on success, error code otherwise.
    // This method never returns EINTR.
    int wait();

    // Block the current thread until the counter reaches 0 or duetime has expired
    // Returns 0 on success, error code otherwise. ETIMEDOUT is for timeout.
    // This method never returns EINTR.
    int timed_wait(const timespec& duetime);

private:
    int *_butex;
    bool _wait_was_invoked;
};

}  // namespace fiber

#endif  // MELON_FIBER_COUNTDOWN_EVENT_H_
