// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/utility/synchronization/condition_variable.h"

#include <errno.h>
#include <sys/time.h>

#include "melon/utility/logging.h"
#include "melon/utility/synchronization/lock.h"
#include "melon/utility/threading/thread_restrictions.h"
#include "melon/utility/time/time.h"

namespace mutil {

ConditionVariable::ConditionVariable(Mutex* user_lock)
    : user_mutex_(user_lock->native_handle()) {
  // NOTE(gejun): Disable monotonic clock always due to difficulty of adapting
  // all versions of gcc
  int rv = pthread_cond_init(&condition_, NULL);
  DMCHECK_EQ(0, rv);
}

ConditionVariable::~ConditionVariable() {
  int rv = pthread_cond_destroy(&condition_);
  DMCHECK_EQ(0, rv);
}

void ConditionVariable::Wait() {
  mutil::ThreadRestrictions::AssertWaitAllowed();
  int rv = pthread_cond_wait(&condition_, user_mutex_);
  DMCHECK_EQ(0, rv);
}

void ConditionVariable::TimedWait(const TimeDelta& max_time) {
  mutil::ThreadRestrictions::AssertWaitAllowed();
  int64_t usecs = max_time.InMicroseconds();
  struct timespec relative_time;
  relative_time.tv_sec = usecs / Time::kMicrosecondsPerSecond;
  relative_time.tv_nsec =
      (usecs % Time::kMicrosecondsPerSecond) * Time::kNanosecondsPerMicrosecond;

#if defined(OS_MACOSX)
  int rv = pthread_cond_timedwait_relative_np(
      &condition_, user_mutex_, &relative_time);
#else
  struct timeval now;
  gettimeofday(&now, NULL);
  struct timespec absolute_time;
  absolute_time.tv_sec = now.tv_sec;
  absolute_time.tv_nsec = now.tv_usec * Time::kNanosecondsPerMicrosecond;

  absolute_time.tv_sec += relative_time.tv_sec;
  absolute_time.tv_nsec += relative_time.tv_nsec;
  absolute_time.tv_sec += absolute_time.tv_nsec / Time::kNanosecondsPerSecond;
  absolute_time.tv_nsec %= Time::kNanosecondsPerSecond;
  DMCHECK_GE(absolute_time.tv_sec, now.tv_sec);  // Overflow paranoia

  int rv = pthread_cond_timedwait(&condition_, user_mutex_, &absolute_time);
#endif  // OS_MACOSX

  DMCHECK(rv == 0 || rv == ETIMEDOUT);
}

void ConditionVariable::Broadcast() {
  int rv = pthread_cond_broadcast(&condition_);
  DMCHECK_EQ(0, rv);
}

void ConditionVariable::Signal() {
  int rv = pthread_cond_signal(&condition_);
  DMCHECK_EQ(0, rv);
}

}  // namespace mutil
