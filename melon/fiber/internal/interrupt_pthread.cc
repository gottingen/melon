
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <signal.h>
#include "melon/fiber/internal/interrupt_pthread.h"

namespace melon::fiber_internal {

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

}  // namespace melon::fiber_internal
