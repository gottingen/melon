// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef MELON_FIBER_KEY_H_
#define MELON_FIBER_KEY_H_

#include <melon/fiber/types.h>
#include <melon/fiber/errno.h>
extern "C" {

// Create a key value identifying a slot in a thread-specific data area.
// Each thread maintains a distinct thread-specific data area.
// `destructor', if non-NULL, is called with the value associated to that key
// when the key is destroyed. `destructor' is not called if the value
// associated is NULL when the key is destroyed.
// Returns 0 on success, error code otherwise.
int fiber_key_create(fiber_key_t *key,
                     void (*destructor)(void *data));

// Delete a key previously returned by fiber_key_create().
// It is the responsibility of the application to free the data related to
// the deleted key in any running thread. No destructor is invoked by
// this function. Any destructor that may have been associated with key
// will no longer be called upon thread exit.
// Returns 0 on success, error code otherwise.
int fiber_key_delete(fiber_key_t key);

// Store `data' in the thread-specific slot identified by `key'.
// fiber_setspecific() is callable from within destructor. If the application
// does so, destructors will be repeatedly called for at most
// PTHREAD_DESTRUCTOR_ITERATIONS times to clear the slots.
// NOTE: If the thread is not created by melon server and lifetime is
// very short(doing a little thing and exit), avoid using fiber-local. The
// reason is that fiber-local always allocate keytable on first call to
// fiber_setspecific, the overhead is negligible in long-lived threads,
// but noticeable in shortly-lived threads. Threads in melon server
// are special since they reuse keytables from a fiber_keytable_pool_t
// in the server.
// Returns 0 on success, error code otherwise.
// If the key is invalid or deleted, return EINVAL.
int fiber_setspecific(fiber_key_t key, void *data);

// Return current value of the thread-specific slot identified by `key'.
// If fiber_setspecific() had not been called in the thread, return NULL.
// If the key is invalid or deleted, return NULL.
void *fiber_getspecific(fiber_key_t key);

void fiber_assign_data(void *data);

void *fiber_get_assigned_data();
}  // extern "C"
#endif  // MELON_FIBER_KEY_H_
