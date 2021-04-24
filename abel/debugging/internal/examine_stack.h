// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//
//

#ifndef ABEL_DEBUGGING_INTERNAL_EXAMINE_STACK_H_
#define ABEL_DEBUGGING_INTERNAL_EXAMINE_STACK_H_

#include "abel/base/profile.h"

namespace abel {

namespace debugging_internal {

// Returns the program counter from signal context, or nullptr if
// unknown. `vuc` is a ucontext_t*. We use void* to avoid the use of
// ucontext_t on non-POSIX systems.
void *GetProgramCounter(void *vuc);

// Uses `writerfn` to dump the program counter, stack trace, and stack
// frame sizes.
void DumpPCAndFrameSizesAndStackTrace(
        void *pc, void *const stack[], int frame_sizes[], int depth,
        int min_dropped_frames, bool symbolize_stacktrace,
        void (*writerfn)(const char *, void *), void *writerfn_arg);

}  // namespace debugging_internal

}  // namespace abel

#endif  // ABEL_DEBUGGING_INTERNAL_EXAMINE_STACK_H_
