
#ifndef  ABEL_SYSTEM_POPEN_H_
#define  ABEL_SYSTEM_POPEN_H_

#include <ostream>
#include <sys/types.h>

namespace abel {

// Read the stdout of child process executing `cmd'.
// Returns the exit status(0-255) of cmd and all the output is stored in
// |os|. -1 otherwise and errno is set appropriately.
int read_command_output(std::ostream& os, const char* cmd);

// Read command line of this program. If `with_args' is true, args are
// included and separated with spaces.
// Returns length of the command line on sucess, -1 otherwise.
// NOTE: `buf' does not end with zero.
    ssize_t read_command_line(char* buf, size_t len, bool with_args);

}

#endif  //ABEL_SYSTEM_POPEN_H_
