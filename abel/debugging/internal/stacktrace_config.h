// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_DEBUGGING_INTERNAL_STACKTRACE_CONFIG_H_
#define ABEL_DEBUGGING_INTERNAL_STACKTRACE_CONFIG_H_

#if defined(ABEL_STACKTRACE_INL_HEADER)
#error ABEL_STACKTRACE_INL_HEADER cannot be directly set

#elif defined(_WIN32)
#define ABEL_STACKTRACE_INL_HEADER \
    "abel/debugging/internal/stacktrace_win32-inl.inc"

#elif defined(__linux__) && !defined(__ANDROID__)

#if !defined(NO_FRAME_POINTER)
# if defined(__i386__) || defined(__x86_64__)
#define ABEL_STACKTRACE_INL_HEADER \
    "abel/debugging/internal/stacktrace_x86-inl.inc"
# elif defined(__ppc__) || defined(__PPC__)
#define ABEL_STACKTRACE_INL_HEADER \
    "abel/debugging/internal/stacktrace_powerpc-inl.inc"
# elif defined(__aarch64__)
#define ABEL_STACKTRACE_INL_HEADER \
    "abel/debugging/internal/stacktrace_aarch64-inl.inc"
# elif defined(__arm__)
// Note: When using glibc this may require -funwind-tables to function properly.
#define ABEL_STACKTRACE_INL_HEADER \
  "abel/debugging/internal/stacktrace_generic-inl.inc"
# else
#define ABEL_STACKTRACE_INL_HEADER \
   "abel/debugging/internal/stacktrace_unimplemented-inl.inc"
# endif
#else  // defined(NO_FRAME_POINTER)
# if defined(__i386__) || defined(__x86_64__) || defined(__aarch64__)
#define ABEL_STACKTRACE_INL_HEADER \
    "abel/debugging/internal/stacktrace_generic-inl.inc"
# elif defined(__ppc__) || defined(__PPC__)
#define ABEL_STACKTRACE_INL_HEADER \
    "abel/debugging/internal/stacktrace_generic-inl.inc"
# else
#define ABEL_STACKTRACE_INL_HEADER \
   "abel/debugging/internal/stacktrace_unimplemented-inl.inc"
# endif
#endif  // NO_FRAME_POINTER

#else
#define ABEL_STACKTRACE_INL_HEADER \
  "abel/debugging/internal/stacktrace_unimplemented-inl.inc"

#endif

#endif  // ABEL_DEBUGGING_INTERNAL_STACKTRACE_CONFIG_H_
