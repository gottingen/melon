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


// Date: Mon. Jan 27  23:08:35 CST 2014

#include <sys/types.h>                          // socket
#include <sys/socket.h>                         // ^
#include <sys/un.h>                             // unix domain socket
#include <melon/utility/fd_guard.h>                     // fd_guard
#include <melon/utility/logging.h>

namespace mutil {

int unix_socket_listen(const char* sockname, bool remove_previous_file) {
    struct sockaddr_un addr;
    addr.sun_family = AF_LOCAL;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sockname);

    fd_guard fd(socket(AF_LOCAL, SOCK_STREAM, 0));
    if (fd < 0) {
        PMLOG(ERROR) << "Fail to create unix socket";
        return -1;
    }
    if (remove_previous_file) {
        remove(sockname);
    }
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        PMLOG(ERROR) << "Fail to bind sockfd=" << fd << " as unix socket="
                    << sockname;
        return -1;
    }
    if (listen(fd, SOMAXCONN) != 0) {
        PMLOG(ERROR) << "Fail to listen to sockfd=" << fd;
        return -1;
    }
    return fd.release();
}

int unix_socket_listen(const char* sockname) {
    return unix_socket_listen(sockname, true);
}

int unix_socket_connect(const char* sockname) {
    struct sockaddr_un addr;
    addr.sun_family = AF_LOCAL;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sockname);

    fd_guard fd(socket(AF_LOCAL, SOCK_STREAM, 0));
    if (fd < 0) {
        PMLOG(ERROR) << "Fail to create unix socket";
        return -1;
    }
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        PMLOG(ERROR) << "Fail to connect to unix socket=" << sockname
                    << " via sockfd=" << fd;
        return -1;
    }
    return fd.release();
}

}  // namespace mutil
