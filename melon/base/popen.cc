//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#include <gflags/gflags.h>
#include <melon/base/build_config.h>
#include <turbo/log/logging.h>
#include <melon/base/macros.h>
#if defined(OS_LINUX)
// clone is a linux specific syscall
#include <sched.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

extern "C" {
int MELON_WEAK fiber_usleep(uint64_t microseconds);
}
#endif

namespace mutil {

#if defined(OS_LINUX)

const int CHILD_STACK_SIZE = 256 * 1024;

struct ChildArgs {
    const char* cmd;
    int pipe_fd0;
    int pipe_fd1;
};

int launch_child_process(void* args) {
    ChildArgs* cargs = (ChildArgs*)args;
    dup2(cargs->pipe_fd1, STDOUT_FILENO);
    close(cargs->pipe_fd0);
    close(cargs->pipe_fd1);
    execl("/bin/sh", "sh", "-c", cargs->cmd, NULL);
    _exit(1);
}

int read_command_output_through_clone(std::ostream& os, const char* cmd) {
    int pipe_fd[2];
    if (pipe(pipe_fd) != 0) {
        PLOG(ERROR) << "Fail to pipe";
        return -1;
    }
    int saved_errno = 0;
    int wstatus = 0;
    pid_t cpid;
    int rc = 0;
    ChildArgs args = { cmd, pipe_fd[0], pipe_fd[1] };
    char buffer[1024];

    char* child_stack = NULL;
    char* child_stack_mem = (char*)malloc(CHILD_STACK_SIZE);
    if (!child_stack_mem) {
        LOG(ERROR) << "Fail to alloc stack for the child process";
        rc = -1;
        goto END;
    }
    child_stack = child_stack_mem + CHILD_STACK_SIZE;  
                               // ^ Assume stack grows downward
    cpid = clone(launch_child_process, child_stack,
                 __WCLONE | CLONE_VM | SIGCHLD | CLONE_UNTRACED, &args);
    if (cpid < 0) {
        PLOG(ERROR) << "Fail to clone child process";
        rc = -1;
        goto END;
    }
    close(pipe_fd[1]);
    pipe_fd[1] = -1;

    for (;;) {
        const ssize_t nr = read(pipe_fd[0], buffer, sizeof(buffer));
        if (nr > 0) {
            os.write(buffer, nr);
            continue;
        } else if (nr == 0) {
            break;
        } else if (errno != EINTR) {
            LOG(ERROR) << "Encountered error while reading for the pipe";
            break;
        }
    }

    close(pipe_fd[0]);
    pipe_fd[0] = -1;

    for (;;) {
        pid_t wpid = waitpid(cpid, &wstatus, WNOHANG | __WALL);
        if (wpid > 0) {
            break;
        }
        if (wpid == 0) {
            if (fiber_usleep != NULL) {
                fiber_usleep(1000);
            } else {
                usleep(1000);
            }
            continue;
        }
        rc = -1;
        goto END;
    }

    if (WIFEXITED(wstatus)) {
        rc = WEXITSTATUS(wstatus);
        goto END;
    }

    if (WIFSIGNALED(wstatus)) {
        os << "Child process(" << cpid << ") was killed by signal "
           << WTERMSIG(wstatus);
    }

    rc = -1;
    errno = ECHILD;

END:
    saved_errno = errno;
    if (child_stack_mem) {
        free(child_stack_mem);
    }
    if (pipe_fd[0] >= 0) {
        close(pipe_fd[0]);
    }
    if (pipe_fd[1] >= 0) {
        close(pipe_fd[1]);
    }
    errno = saved_errno;
    return rc;
}

DEFINE_bool(run_command_through_clone, false,
            "(Linux specific) Run command with clone syscall to "
            "avoid the costly page table duplication");

#endif // OS_LINUX

#include <stdio.h>

int read_command_output_through_popen(std::ostream& os, const char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (pipe == NULL) {
        return -1;
    }
    char buffer[1024];
    for (;;) {
        size_t nr = fread(buffer, 1, sizeof(buffer), pipe);
        if (nr != 0) {
            os.write(buffer, nr);
        }
        if (nr != sizeof(buffer)) {
            if (feof(pipe)) {
                break;
            } else if (ferror(pipe)) {
                LOG(ERROR) << "Encountered error while reading for the pipe";
                break;
            }
            // retry;
        }
    }

    const int wstatus = pclose(pipe);

    if (wstatus < 0) {
        return wstatus;
    }
    if (WIFEXITED(wstatus)) {
        return WEXITSTATUS(wstatus);
    }
    if (WIFSIGNALED(wstatus)) {
        os << "Child process was killed by signal "
           << WTERMSIG(wstatus);
    }
    errno = ECHILD;
    return -1;
}

int read_command_output(std::ostream& os, const char* cmd) {
#if !defined(OS_LINUX)
    return read_command_output_through_popen(os, cmd);
#else
    return FLAGS_run_command_through_clone
        ? read_command_output_through_clone(os, cmd)
        : read_command_output_through_popen(os, cmd);
#endif
}

}  // namespace mutil
