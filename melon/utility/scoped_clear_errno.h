// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MUTIL_SCOPED_CLEAR_ERRNO_H_
#define MUTIL_SCOPED_CLEAR_ERRNO_H_

#include <errno.h>

#include "melon/utility/basictypes.h"

namespace mutil {

// Simple scoper that saves the current value of errno, resets it to 0, and on
// destruction puts the old value back.
class ScopedClearErrno {
 public:
  ScopedClearErrno() : old_errno_(errno) {
    errno = 0;
  }
  ~ScopedClearErrno() {
    if (errno == 0)
      errno = old_errno_;
  }

 private:
  const int old_errno_;

  DISALLOW_COPY_AND_ASSIGN(ScopedClearErrno);
};

}  // namespace mutil

#endif  // MUTIL_SCOPED_CLEAR_ERRNO_H_
