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



#include <melon/utility/build_config.h>

#if defined(OS_LINUX)

#include <sys/epoll.h>                             // epoll_create
#include <sys/types.h>                             // socketpair
#include <sys/socket.h>                            // ^
#include <melon/utility/fd_guard.h>                         // fd_guard
#include <melon/rpc/details/has_epollrdhup.h>

#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

namespace melon {

static unsigned int check_epollrdhup() {
    mutil::fd_guard epfd(epoll_create(16));
    if (epfd < 0) {
        return 0;
    }
    mutil::fd_guard fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, (int*)fds) < 0) {
        return 0;
    }
    epoll_event evt = { static_cast<uint32_t>(EPOLLIN | EPOLLRDHUP | EPOLLET),
                        { NULL }};
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fds[0], &evt) < 0) {
        return 0;
    }
    if (close(fds[1].release()) < 0) {
        return 0;
    }
    epoll_event e;
    int n;
    while ((n = epoll_wait(epfd, &e, 1, -1)) == 0);
    if (n < 0) {
        return 0;
    }
    return (e.events & EPOLLRDHUP) ? EPOLLRDHUP : static_cast<EPOLL_EVENTS>(0);
}

extern const unsigned int has_epollrdhup = check_epollrdhup();

} // namespace melon

#else

namespace melon {
extern const unsigned int has_epollrdhup = false;
}

#endif // defined(OS_LINUX)
