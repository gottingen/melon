// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a cross platform interface for helper functions related to
// debuggers.  You should use this to test if you're running under a debugger,
// and if you would like to yield (breakpoint) into the debugger.

#ifndef MUTIL_DEBUG_DEBUGGER_H
#define MUTIL_DEBUG_DEBUGGER_H

#include "melon/utility/base_export.h"

namespace mutil {
namespace debug {

// Waits wait_seconds seconds for a debugger to attach to the current process.
// When silent is false, an exception is thrown when a debugger is detected.
MUTIL_EXPORT bool WaitForDebugger(int wait_seconds, bool silent);

// Returns true if the given process is being run under a debugger.
//
// On OS X, the underlying mechanism doesn't work when the sandbox is enabled.
// To get around this, this function caches its value.
//
// WARNING: Because of this, on OS X, a call MUST be made to this function
// BEFORE the sandbox is enabled.
MUTIL_EXPORT bool BeingDebugged();

// Break into the debugger, assumes a debugger is present.
MUTIL_EXPORT void BreakDebugger();

// Used in test code, this controls whether showing dialogs and breaking into
// the debugger is suppressed for debug errors, even in debug mode (normally
// release mode doesn't do this stuff --  this is controlled separately).
// Normally UI is not suppressed.  This is normally used when running automated
// tests where we want a crash rather than a dialog or a debugger.
MUTIL_EXPORT void SetSuppressDebugUI(bool suppress);
MUTIL_EXPORT bool IsDebugUISuppressed();

}  // namespace debug
}  // namespace mutil

#endif  // MUTIL_DEBUG_DEBUGGER_H
