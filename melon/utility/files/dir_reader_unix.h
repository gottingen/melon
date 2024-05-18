// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//



#ifndef MUTIL_FILES_DIR_READER_UNIX_H_
#define MUTIL_FILES_DIR_READER_UNIX_H_

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <dirent.h>

#include "melon/utility/logging.h"
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