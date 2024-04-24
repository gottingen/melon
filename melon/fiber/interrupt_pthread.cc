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


#include <signal.h>
#include "melon/fiber/interrupt_pthread.h"

namespace fiber {

// TODO: Make sure SIGURG is not used by user.
// This empty handler is simply for triggering EINTR in blocking syscalls.
void do_nothing_handler(int) {}

static pthread_once_t register_sigurg_once = PTHREAD_ONCE_INIT;

static void register_sigurg() {
    signal(SIGURG, do_nothing_handler);
}

int interrupt_pthread(pthread_t th) {
    pthread_once(&register_sigurg_once, register_sigurg);
    return pthread_kill(th, SIGURG);
}

}  // namespace fiber
