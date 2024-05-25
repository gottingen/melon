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

// Watch timestamp of a file

#ifndef MUTIL_FILES_FILE_WATCHER_H
#define MUTIL_FILES_FILE_WATCHER_H

#include <stdint.h>                                 // int64_t
#include <string>                                   // std::string

// Example:
//   FileWatcher fw;
//   fw.init("to_be_watched_file");
//   ....
//   if (fw.check_and_consume() > 0) {
//       // the file is created or updated 
//       ......
//   }

namespace mutil {
class FileWatcher {
public:
    enum Change {
        DELETED = -1,
        UNCHANGED = 0,
        UPDATED = 1,
        CREATED = 2,
    };

    typedef int64_t Timestamp;
    
    FileWatcher();

    // Watch file at `file_path', must be called before calling other methods.
    // Returns 0 on success, -1 otherwise.
    int init(const char* file_path);
    // Let check_and_consume returns CREATE when file_path already exists.
    int init_from_not_exist(const char* file_path);

    // Check and consume change of the watched file. Write `last_timestamp'
    // if it's not NULL.
    // Returns:
    //   CREATE    the file is created since last call to this method.
    //   UPDATED   the file is modified since last call.
    //   UNCHANGED the file has no change since last call.
    //   DELETED   the file was deleted since last call.
    // Note: If the file is updated too frequently, this method may return 
    // UNCHANGED due to precision of stat(2) and the file system. If the file
    // is created and deleted too frequently, the event may not be detected.
    Change check_and_consume(Timestamp* last_timestamp = NULL);

    // Set internal timestamp. User can use this method to make
    // check_and_consume() replay the change.
    void restore(Timestamp timestamp);
    
    // Get path of watched file
    const char* filepath() const { return _file_path.c_str(); }

private:
    Change check(Timestamp* new_timestamp) const;

    std::string _file_path;
    Timestamp _last_ts;    
};
}  // namespace mutil

#endif  // MUTIL_FILES_FILE_WATCHER_H
