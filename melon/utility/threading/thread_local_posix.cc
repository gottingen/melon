// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/utility/threading/thread_local.h"

#include <pthread.h>

#include <melon/utility/logging.h>

#if !defined(OS_ANDROID)

namespace mutil {
namespace internal {

// static
void ThreadLocalPlatform::AllocateSlot(SlotType* slot) {
  int error = pthread_key_create(slot, NULL);
  MCHECK_EQ(error, 0);
}

// static
void ThreadLocalPlatform::FreeSlot(SlotType slot) {
  int error = pthread_key_delete(slot);
  DMCHECK_EQ(0, error);
}

// static
void* ThreadLocalPlatform::GetValueFromSlot(SlotType slot) {
  return pthread_getspecific(slot);
}

// static
void ThreadLocalPlatform::SetValueInSlot(SlotType slot, void* value) {
  int error = pthread_setspecific(slot, value);
  DMCHECK_EQ(error, 0);
}

}  // namespace internal
}  // namespace mutil

#endif  // !defined(OS_ANDROID)
