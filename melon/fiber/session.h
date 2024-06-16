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


#ifndef MELON_FIBER_SESSION_H_
#define MELON_FIBER_SESSION_H_

#include <melon/base/macros.h>              // MELON_SYMBOLSTR
#include <melon/fiber/types.h>

__BEGIN_DECLS

// ----------------------------------------------------------------------
// Functions to create 64-bit identifiers that can be attached with data
// and locked without ABA issues. All functions can be called from
// multiple threads simultaneously. Notice that fiber_session_t is designed
// for managing a series of non-heavily-contended actions on an object.
// It's slower than mutex and not proper for general synchronizations.
// ----------------------------------------------------------------------

// Create a fiber_session_t and put it into *id. Crash when `id' is NULL.
// id->value will never be zero.
// `on_error' will be called after fiber_session_error() is called.
// -------------------------------------------------------------------------
// ! User must call fiber_session_unlock() or fiber_session_unlock_and_destroy()
// ! inside on_error.
// -------------------------------------------------------------------------
// Returns 0 on success, error code otherwise.
int fiber_session_create(
    fiber_session_t* id, void* data,
    int (*on_error)(fiber_session_t id, void* data, int error_code));

// When this function is called successfully, *id, *id+1 ... *id + range - 1
// are mapped to same internal entity. Operations on any of the id work as
// if they're manipulating a same id. `on_error' is called with the id issued
// by corresponding fiber_session_error(). This is designed to let users encode
// versions into identifiers.
// `range' is limited inside [1, 1024].
int fiber_session_create_ranged(
    fiber_session_t* id, void* data,
    int (*on_error)(fiber_session_t id, void* data, int error_code),
    int range);

// Wait until `id' being destroyed.
// Waiting on a destroyed fiber_session_t returns immediately.
// Returns 0 on success, error code otherwise.
int fiber_session_join(fiber_session_t id);

// Destroy a created but never-used fiber_session_t.
// Returns 0 on success, EINVAL otherwise.
int fiber_session_cancel(fiber_session_t id);

// Issue an error to `id'.
// If `id' is not locked, lock the id and run `on_error' immediately. Otherwise
// `on_error' will be called with the error just before `id' being unlocked.
// If `id' is destroyed, un-called on_error are dropped.
// Returns 0 on success, error code otherwise.
#define fiber_session_error(id, err)                                        \
    fiber_session_error_verbose(id, err, __FILE__ ":" MELON_SYMBOLSTR(__LINE__))

int fiber_session_error_verbose(fiber_session_t id, int error_code,
                             const char *location);

// Make other fiber_session_lock/fiber_session_trylock on the id fail, the id must
// already be locked. If the id is unlocked later rather than being destroyed,
// effect of this function is cancelled. This function avoids useless
// waiting on a fiber_session which will be destroyed soon but still needs to
// be joinable.
// Returns 0 on success, error code otherwise.
int fiber_session_about_to_destroy(fiber_session_t id);

// Try to lock `id' (for using the data exclusively)
// On success return 0 and set `pdata' with the `data' parameter to
// fiber_session_create[_ranged], EBUSY on already locked, error code otherwise.
int fiber_session_trylock(fiber_session_t id, void** pdata);

// Lock `id' (for using the data exclusively). If `id' is locked
// by others, wait until `id' is unlocked or destroyed.
// On success return 0 and set `pdata' with the `data' parameter to
// fiber_session_create[_ranged], error code otherwise.
#define fiber_session_lock(id, pdata)                                      \
    fiber_session_lock_verbose(id, pdata, __FILE__ ":" MELON_SYMBOLSTR(__LINE__))
int fiber_session_lock_verbose(fiber_session_t id, void** pdata,
                            const char *location);

// Lock `id' (for using the data exclusively) and reset the range. If `id' is 
// locked by others, wait until `id' is unlocked or destroyed. if `range' is
// smaller than the original range of this id, nothing happens about the range
#define fiber_session_lock_and_reset_range(id, pdata, range)               \
    fiber_session_lock_and_reset_range_verbose(id, pdata, range,           \
                               __FILE__ ":" MELON_SYMBOLSTR(__LINE__))
int fiber_session_lock_and_reset_range_verbose(
    fiber_session_t id, void **pdata,
    int range, const char *location);

// Unlock `id'. Must be called after a successful call to fiber_session_trylock()
// or fiber_session_lock().
// Returns 0 on success, error code otherwise.
int fiber_session_unlock(fiber_session_t id);

// Unlock and destroy `id'. Waiters blocking on fiber_session_join() or
// fiber_session_lock() will wake up. Must be called after a successful call to
// fiber_session_trylock() or fiber_session_lock().
// Returns 0 on success, error code otherwise.
int fiber_session_unlock_and_destroy(fiber_session_t id);

// **************************************************************************
// fiber_session_list_xxx functions are NOT thread-safe unless explicitly stated

// Initialize a list for storing fiber_session_t. When an id is destroyed, it will
// be removed from the list automatically.
// The commented parameters are not used anymore and just kept to not break
// compatibility.
int fiber_session_list_init(fiber_session_list_t* list,
                         unsigned /*size*/,
                         unsigned /*conflict_size*/);
// Destroy the list.
void fiber_session_list_destroy(fiber_session_list_t* list);

// Add a fiber_session_t into the list.
int fiber_session_list_add(fiber_session_list_t* list, fiber_session_t id);

// Swap internal fields of two lists.
void fiber_session_list_swap(fiber_session_list_t* dest,
                          fiber_session_list_t* src);

// Issue error_code to all fiber_session_t inside `list' and clear `list'.
// Notice that this function iterates all id inside the list and may call
// on_error() of each valid id in-place, in another word, this thread-unsafe
// function is not suitable to be enclosed within a lock directly.
// To make the critical section small, swap the list inside the lock and
// reset the swapped list outside the lock as follows:
//   fiber_session_list_t tmplist;
//   fiber_session_list_init(&tmplist, 0, 0);
//   LOCK;
//   fiber_session_list_swap(&tmplist, &the_list_to_reset);
//   UNLOCK;
//   fiber_session_list_reset(&tmplist, error_code);
//   fiber_session_list_destroy(&tmplist);
int fiber_session_list_reset(fiber_session_list_t* list, int error_code);
// Following 2 functions wrap above process.
int fiber_session_list_reset_pthreadsafe(
    fiber_session_list_t* list, int error_code, pthread_mutex_t* mutex);
int fiber_session_list_reset_fibersafe(
    fiber_session_list_t* list, int error_code, fiber_mutex_t* mutex);

__END_DECLS

#if defined(__cplusplus)
// cpp specific API, with an extra `error_text' so that error information
// is more comprehensive
int fiber_session_create2(
    fiber_session_t* id, void* data,
    int (*on_error)(fiber_session_t id, void* data, int error_code,
                    const std::string& error_text));
int fiber_session_create2_ranged(
    fiber_session_t* id, void* data,
    int (*on_error)(fiber_session_t id, void* data, int error_code,
                    const std::string& error_text),
    int range);
#define fiber_session_error2(id, ec, et)                                   \
    fiber_session_error2_verbose(id, ec, et, __FILE__ ":" MELON_SYMBOLSTR(__LINE__))
int fiber_session_error2_verbose(fiber_session_t id, int error_code,
                              const std::string& error_text,
                              const char *location);
int fiber_session_list_reset2(fiber_session_list_t* list, int error_code,
                           const std::string& error_text);
int fiber_session_list_reset2_pthreadsafe(fiber_session_list_t* list, int error_code,
                                       const std::string& error_text,
                                       pthread_mutex_t* mutex);
int fiber_session_list_reset2_fibersafe(fiber_session_list_t* list, int error_code,
                                       const std::string& error_text,
                                       fiber_mutex_t* mutex);
#endif

#endif  // MELON_FIBER_SESSION_H_
