// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/utility/threading/thread_local_storage.h"

#include <turbo/log/logging.h>

namespace mutil {

namespace internal {

bool PlatformThreadLocalStorage::AllocTLS(TLSKey* key) {
  return !pthread_key_create(key,
      mutil::internal::PlatformThreadLocalStorage::OnThreadExit);
}

void PlatformThreadLocalStorage::FreeTLS(TLSKey key) {
  int ret = pthread_key_delete(key);
  DCHECK_EQ(ret, 0);
}

void* PlatformThreadLocalStorage::GetTLSValue(TLSKey key) {
  return pthread_getspecific(key);
}

void PlatformThreadLocalStorage::SetTLSValue(TLSKey key, void* value) {
  int ret = pthread_setspecific(key, value);
  DCHECK_EQ(ret, 0);
}

}  // namespace internal

}  // namespace mutil
