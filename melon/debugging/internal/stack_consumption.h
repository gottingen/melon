
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
//

// Helper function for measuring stack consumption of signal handlers.

#ifndef MELON_DEBUGGING_INTERNAL_STACK_CONSUMPTION_H_
#define MELON_DEBUGGING_INTERNAL_STACK_CONSUMPTION_H_

#include "melon/base/profile.h"

// The code in this module is not portable.
// Use this feature test macro to detect its availability.
#ifdef MELON_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION
#error MELON_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION cannot be set directly
#elif !defined(__APPLE__) && !defined(_WIN32) && \
    (defined(__i386__) || defined(__x86_64__) || defined(__ppc__))
#define MELON_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION 1

namespace melon::debugging {

namespace debugging_internal {

// Returns the stack consumption in bytes for the code exercised by
// signal_handler.  To measure stack consumption, signal_handler is registered
// as a signal handler, so the code that it exercises must be async-signal
// safe.  The argument of signal_handler is an implementation detail of signal
// handlers and should ignored by the code for signal_handler.  Use global
// variables to pass information between your test code and signal_handler.
int GetSignalHandlerStackConsumption(void (*signal_handler)(int));

}  // namespace debugging_internal

}  // namespace melon::debugging

#endif  // MELON_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION

#endif  // MELON_DEBUGGING_INTERNAL_STACK_CONSUMPTION_H_
