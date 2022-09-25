
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
// -----------------------------------------------------------------------------
// File: symbolize.h
// -----------------------------------------------------------------------------
//
// This file configures the melon symbolizer for use in converting instruction
// pointer addresses (program counters) into human-readable names (function
// calls, etc.) within melon code.
//
// The symbolizer may be invoked from several sources:
//
//   * Implicitly, through the installation of an melon failure signal handler.
//     (See failure_signal_handler.h for more information.)
//   * By calling `symbolize()` directly on a program counter you obtain through
//     `melon::debugging::get_stack_trace()` or `melon::debugging::get_stack_frames()`. (See stacktrace.h
//     for more information.
//   * By calling `symbolize()` directly on a program counter you obtain through
//     other means (which would be platform-dependent).
//
// In all of the above cases, the symbolizer must first be initialized before
// any program counter values can be symbolized. If you are installing a failure
// signal handler, initialize the symbolizer before you do so.
//
// Example:
//
//   int main(int argc, char** argv) {
//     // Initialize the Symbolizer before installing the failure signal handler
//     melon::debugging::initialize_symbolizer(argv[0]);
//
//     // Now you may install the failure signal handler
//     melon::debugging::failure_signal_handler_options options;
//     melon::debugging::install_failure_signal_handler(options);
//
//     // Start running your main program
//     ...
//     return 0;
//  }
//
#ifndef MELON_DEBUGGING_SYMBOLIZE_H_
#define MELON_DEBUGGING_SYMBOLIZE_H_

#include "melon/debugging/internal/symbolize.h"

namespace melon::debugging {


    // initialize_symbolizer()
    //
    // Initializes the program counter symbolizer, given the path of the program
    // (typically obtained through `main()`s `argv[0]`). The melon symbolizer
    // allows you to read program counters (instruction pointer values) using their
    // human-readable names within output such as stack traces.
    //
    // Example:
    //
    // int main(int argc, char *argv[]) {
    //   melon::debugging::initialize_symbolizer(argv[0]);
    //   // Now you can use the symbolizer
    // }
    void initialize_symbolizer(const char *argv0);

    // symbolize()
    //
    // Symbolizes a program counter (instruction pointer value) `pc` and, on
    // success, writes the name to `out`. The symbol name is demangled, if possible.
    // Note that the symbolized name may be truncated and will be NUL-terminated.
    // Demangling is supported for symbols generated by GCC 3.x or newer). Returns
    // `false` on failure.
    //
    // Example:
    //
    //   // Print a program counter and its symbol name.
    //   static void DumpPCAndSymbol(void *pc) {
    //     char tmp[1024];
    //     const char *symbol = "(unknown)";
    //     if (melon::debugging::symbolize(pc, tmp, sizeof(tmp))) {
    //       symbol = tmp;
    //     }
    //     melon::debugging::printf("%*p  %s\n", pc, symbol);
    //  }
    bool symbolize(const void *pc, char *out, int out_size);


}  // namespace melon::debugging

#endif  // MELON_DEBUGGING_SYMBOLIZE_H_