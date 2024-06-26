// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MUTIL_THREADING_NON_THREAD_SAFE_IMPL_H_
#define MUTIL_THREADING_NON_THREAD_SAFE_IMPL_H_

#include "melon/utility/base_export.h"
#include "melon/utility/threading/thread_checker_impl.h"

namespace mutil {

// Full implementation of NonThreadSafe, for debug mode or for occasional
// temporary use in release mode e.g. when you need to CHECK on a thread
// bug that only occurs in the wild.
//
// Note: You should almost always use the NonThreadSafe class to get
// the right version of the class for your build configuration.
class MUTIL_EXPORT NonThreadSafeImpl {
 public:
  bool CalledOnValidThread() const;

 protected:
  ~NonThreadSafeImpl();

  // Changes the thread that is checked for in CalledOnValidThread. The next
  // call to CalledOnValidThread will attach this class to a new thread. It is
  // up to the NonThreadSafe derived class to decide to expose this or not.
  // This may be useful when an object may be created on one thread and then
  // used exclusively on another thread.
  void DetachFromThread();

 private:
  ThreadCheckerImpl thread_checker_;
};

}  // namespace mutil

#endif  // MUTIL_THREADING_NON_THREAD_SAFE_IMPL_H_
