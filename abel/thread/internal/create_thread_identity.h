/*
 * Copyright 2017 The abel Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Interface for getting the current thread_identity, creating one if necessary.
// See thread_identity.h.
//
// This file is separate from thread_identity.h because creating a new
// thread_identity requires slightly higher level libraries (per_thread_sem
// and low_level_alloc) than accessing an existing one.  This separation allows
// us to have a smaller //abel/base:base.

#ifndef ABEL_SYNCHRONIZATION_INTERNAL_CREATE_THREAD_IDENTITY_H_
#define ABEL_SYNCHRONIZATION_INTERNAL_CREATE_THREAD_IDENTITY_H_

#include <abel/thread/internal/thread_identity.h>
#include <abel/base/profile.h>

namespace abel {

    namespace thread_internal {

// Allocates and attaches a thread_identity object for the calling thread.
// For private use only.
        thread_internal::thread_identity *create_thread_identity();

// A per-thread destructor for reclaiming associated thread_identity objects.
// For private use only.
        void reclaim_thread_identity(void *v);

// Returns the thread_identity object representing the calling thread; guaranteed
// to be unique for its lifetime.  The returned object will remain valid for the
// program's lifetime; although it may be re-assigned to a subsequent thread.
// If one does not exist for the calling thread, allocate it now.
        ABEL_FORCE_INLINE thread_internal::thread_identity *get_or_create_current_thread_identity() {
            thread_internal::thread_identity *identity =
                    thread_internal::CurrentThreadIdentityIfPresent();
            if (ABEL_UNLIKELY(identity == nullptr)) {
                return create_thread_identity();
            }
            return identity;
        }

    }  // namespace thread_internal

}  // namespace abel

#endif  // ABEL_SYNCHRONIZATION_INTERNAL_CREATE_THREAD_IDENTITY_H_
