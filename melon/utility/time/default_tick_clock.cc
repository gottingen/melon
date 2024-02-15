// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/utility/time/default_tick_clock.h"

namespace mutil {

DefaultTickClock::~DefaultTickClock() {}

TimeTicks DefaultTickClock::NowTicks() {
  return TimeTicks::Now();
}

}  // namespace mutil
