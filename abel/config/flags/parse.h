//
//
// -----------------------------------------------------------------------------
// File: parse.h
// -----------------------------------------------------------------------------
//
// This file defines the main parsing function for abel flags:
// `abel::parse_command_line()`.

#ifndef ABEL_FLAGS_PARSE_H_
#define ABEL_FLAGS_PARSE_H_

#include <string>
#include <vector>

#include <abel/config/flags/internal/parse.h>

namespace abel {


// parse_command_line()
//
// Parses the set of command-line arguments passed in the `argc` (argument
// count) and `argv[]` (argument vector) parameters from `main()`, assigning
// values to any defined abel flags. (Any arguments passed after the
// flag-terminating delimiter (`--`) are treated as positional arguments and
// ignored.)
//
// Any command-line flags (and arguments to those flags) are parsed into abel
// Flag values, if those flags are defined. Any undefined flags will either
// return an error, or be ignored if that flag is designated using `undefok` to
// indicate "undefined is OK."
//
// Any command-line positional arguments not part of any command-line flag (or
// arguments to a flag) are returned in a vector, with the program invocation
// name at position 0 of that vector. (Note that this includes positional
// arguments after the flag-terminating delimiter `--`.)
//
// After all flags and flag arguments are parsed, this function looks for any
// built-in usage flags (e.g. `--help`), and if any were specified, it reports
// help messages and then exits the program.
    std::vector<char *> parse_command_line(int argc, char *argv[]);


}  // namespace abel

#endif  // ABEL_FLAGS_PARSE_H_
