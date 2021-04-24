// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_SYSTEM_SCOPED_ERRNO_H_
#define ABEL_SYSTEM_SCOPED_ERRNO_H_


#include "abel/base/profile.h"
#include <errno.h>

namespace abel {
class scoped_errno {
  public:
    scoped_errno() : _saved_errno(errno) {
        errno = 0;
    }

    ~scoped_errno() {
        if (errno == 0)
            errno = _saved_errno;
    }

  private:
    const int _saved_errno;ABEL_NON_COPYABLE(scoped_errno);
};
}  // namespace abel

#endif  // ABEL_SYSTEM_SCOPED_ERRNO_H_
