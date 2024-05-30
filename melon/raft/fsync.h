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
#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <melon/raft/storage.h>
#include <melon/raft/config.h>

namespace melon::raft {

    inline int raft_fsync(int fd) {
        if (FLAGS_raft_use_fsync_rather_than_fdatasync) {
            return fsync(fd);
        } else {
#ifdef __APPLE__
            return fcntl(fd, F_FULLFSYNC);
#else
            return fdatasync(fd);
#endif
        }
    }

    inline bool raft_sync_meta() {
        return FLAGS_raft_sync || FLAGS_raft_sync_meta;
    }

}  //  namespace melon::raft

