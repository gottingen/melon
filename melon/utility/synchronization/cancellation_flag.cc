// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/utility/synchronization/cancellation_flag.h"

#include "melon/utility/logging.h"

namespace mutil {

void CancellationFlag::Set() {
#if !defined(NDEBUG)
  DMCHECK_EQ(set_on_, PlatformThread::CurrentId());
#endif
  mutil::subtle::Release_Store(&flag_, 1);
}

bool CancellationFlag::IsSet() const {
  return mutil::subtle::Acquire_Load(&flag_) != 0;
}

}  // namespace mutil
