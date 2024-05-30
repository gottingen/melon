//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
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
