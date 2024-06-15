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


// Date: 2010/05/29

#include <sys/stat.h>
#include <melon/base/build_config.h>              // OS_MACOSX
#include <melon/utility/files/file_watcher.h>

namespace mutil {

static const FileWatcher::Timestamp NON_EXIST_TS =
    static_cast<FileWatcher::Timestamp>(-1);

FileWatcher::FileWatcher() : _last_ts(NON_EXIST_TS) {
}

int FileWatcher::init(const char* file_path) {
    if (init_from_not_exist(file_path) != 0) {
        return -1;
    }
    check_and_consume(NULL);
    return 0;
}

int FileWatcher::init_from_not_exist(const char* file_path) {
    if (NULL == file_path) {
        return -1;
    }
    if (!_file_path.empty()) {
        return -1;
    }
    _file_path = file_path;
    return 0;
}

FileWatcher::Change FileWatcher::check(Timestamp* new_timestamp) const {
    struct stat st;
    const int ret = stat(_file_path.c_str(), &st);
    if (ret < 0) {
        *new_timestamp = NON_EXIST_TS;
        if (NON_EXIST_TS != _last_ts) {
            return DELETED;
        } else {
            return UNCHANGED;
        }
    } else {
        // Use microsecond timestamps which can be used for:
        //   2^63 / 1000000 / 3600 / 24 / 365 = 292471 years
        const Timestamp cur_ts =
#if defined(OS_MACOSX)
            st.st_mtimespec.tv_sec * 1000000L + st.st_mtimespec.tv_nsec / 1000L;
#else
            st.st_mtim.tv_sec * 1000000L + st.st_mtim.tv_nsec / 1000L;
#endif
        *new_timestamp = cur_ts;
        if (NON_EXIST_TS != _last_ts) {
            if (cur_ts != _last_ts) {
                return UPDATED;
            } else {
                return UNCHANGED;
            }
        } else {
            return CREATED;
        }
    }
}

FileWatcher::Change FileWatcher::check_and_consume(Timestamp* last_timestamp) {
    Timestamp new_timestamp;
    Change e = check(&new_timestamp);
    if (last_timestamp) {
        *last_timestamp = _last_ts;
    }
    if (e != UNCHANGED) {
        _last_ts = new_timestamp;
    }
    return e;
}

void FileWatcher::restore(Timestamp timestamp) {
    _last_ts = timestamp;
}

}  // namespace mutil
