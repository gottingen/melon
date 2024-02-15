// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MUTIL_DEFAULT_CLOCK_H_
#define MUTIL_DEFAULT_CLOCK_H_

#include "melon/utility/base_export.h"
#include "melon/utility/compiler_specific.h"
#include "melon/utility/time/clock.h"

namespace mutil {

// DefaultClock is a Clock implementation that uses Time::Now().
class MUTIL_EXPORT DefaultClock : public Clock {
 public:
  virtual ~DefaultClock();

  // Simply returns Time::Now().
  virtual Time Now() OVERRIDE;
};

}  // namespace mutil

#endif  // MUTIL_DEFAULT_CLOCK_H_
