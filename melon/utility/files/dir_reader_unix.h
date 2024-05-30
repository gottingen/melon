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



#ifndef MUTIL_FILES_DIR_READER_UNIX_H_
#define MUTIL_FILES_DIR_READER_UNIX_H_

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <dirent.h>

#include <turbo/log/logging.h>
#include "melon/utility/posix/eintr_wrapper.h"

// See the comments in dir_reader_posix.h about this.

namespace mutil {

class DirReaderUnix {
 public:
  explicit DirReaderUnix(const char* directory_path)
      : fd_(open(directory_path, O_RDONLY | O_DIRECTORY)),
        dir_(NULL),current_(NULL) {
      dir_ = fdopendir(fd_);
  }

  ~DirReaderUnix() {
    if (NULL != dir_) {
      if (IGNORE_EINTR(closedir(dir_)) == 0) { // this implicitly closes fd_
        dir_ = NULL;
      } else {
        RAW_LOG(ERROR, "Failed to close directory.");
      }
    }
  }

  bool IsValid() const {
    return dir_ != NULL;
  }

  // Move to the next entry returning false if the iteration is complete.
  bool Next() {
    int err = readdir_r(dir_,&entry_, &current_);
    if(0 != err || NULL == current_){
        return false;
    }
    return true;
  }

  const char* name() const {
    if (NULL == current_)
      return NULL;
    return current_->d_name;
  }

  int fd() const {
    return fd_;
  }

  static bool IsFallback() {
    return false;
  }

 private:
  const int fd_;
  DIR* dir_;
  struct dirent entry_;
  struct dirent* current_;
};

}  // namespace mutil

#endif // MUTIL_FILES_DIR_READER_LINUX_H_
