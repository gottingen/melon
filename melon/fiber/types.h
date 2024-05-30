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

#ifndef MELON_FIBER_TYPES_H_
#define MELON_FIBER_TYPES_H_

#include <stdint.h>                            // uint64_t
#if defined(__cplusplus)
#include <turbo/log/logging.h>                      // CHECK
#endif

typedef uint64_t fiber_t;

// tid returned by fiber_start_* never equals this value.
static const fiber_t INVALID_FIBER = 0;

// fiber tag default is 0
typedef int fiber_tag_t;
static const fiber_tag_t FIBER_TAG_INVALID = -1;
static const fiber_tag_t FIBER_TAG_DEFAULT = 0;

struct sockaddr;

typedef unsigned fiber_stacktype_t;
static const fiber_stacktype_t FIBER_STACKTYPE_UNKNOWN = 0;
static const fiber_stacktype_t FIBER_STACKTYPE_PTHREAD = 1;
static const fiber_stacktype_t FIBER_STACKTYPE_SMALL = 2;
static const fiber_stacktype_t FIBER_STACKTYPE_NORMAL = 3;
static const fiber_stacktype_t FIBER_STACKTYPE_LARGE = 4;

typedef unsigned fiber_attrflags_t;
static const fiber_attrflags_t FIBER_LOG_START_AND_FINISH = 8;
static const fiber_attrflags_t FIBER_LOG_CONTEXT_SWITCH = 16;
static const fiber_attrflags_t FIBER_NOSIGNAL = 32;
static const fiber_attrflags_t FIBER_NEVER_QUIT = 64;
static const fiber_attrflags_t FIBER_INHERIT_SPAN = 128;

// Key of thread-local data, created by fiber_key_create.
typedef struct {
    uint32_t index;    // index in KeyTable
    uint32_t version;  // ABA avoidance
} fiber_key_t;

static const fiber_key_t INVALID_FIBER_KEY = { 0, 0 };

#if defined(__cplusplus)
// Overload operators for fiber_key_t
inline bool operator==(fiber_key_t key1, fiber_key_t key2)
{ return key1.index == key2.index && key1.version == key2.version; }
inline bool operator!=(fiber_key_t key1, fiber_key_t key2)
{ return !(key1 == key2); }
inline bool operator<(fiber_key_t key1, fiber_key_t key2) {
    return key1.index != key2.index ? (key1.index < key2.index) :
        (key1.version < key2.version);
}
inline bool operator>(fiber_key_t key1, fiber_key_t key2)
{ return key2 < key1; }
inline bool operator<=(fiber_key_t key1, fiber_key_t key2)
{ return !(key2 < key1); }
inline bool operator>=(fiber_key_t key1, fiber_key_t key2)
{ return !(key1 < key2); }
inline std::ostream& operator<<(std::ostream& os, fiber_key_t key) {
    return os << "fiber_key_t{index=" << key.index << " version="
              << key.version << '}';
}
#endif  // __cplusplus

typedef struct {
    pthread_mutex_t mutex;
    void* free_keytables;
    int destroyed;
} fiber_keytable_pool_t;

typedef struct {
    size_t nfree;
} fiber_keytable_pool_stat_t;

// Attributes for thread creation.
typedef struct fiber_attr_t {
    fiber_stacktype_t stack_type;
    fiber_attrflags_t flags;
    fiber_keytable_pool_t* keytable_pool;
    fiber_tag_t tag;

#if defined(__cplusplus)
    void operator=(unsigned stacktype_and_flags) {
        stack_type = (stacktype_and_flags & 7);
        flags = (stacktype_and_flags & ~(unsigned)7u);
        keytable_pool = NULL;
        tag = FIBER_TAG_INVALID;
    }
    fiber_attr_t operator|(unsigned other_flags) const {
        CHECK(!(other_flags & 7)) << "flags=" << other_flags;
        fiber_attr_t tmp = *this;
        tmp.flags |= (other_flags & ~(unsigned)7u);
        return tmp;
    }
#endif  // __cplusplus
} fiber_attr_t;

// fibers started with this attribute will run on stack of worker pthread and
// all fiber functions that would block the fiber will block the pthread.
// The fiber will not allocate its own stack, simply occupying a little meta
// memory. This is required to run JNI code which checks layout of stack. The
// obvious drawback is that you need more worker pthreads when you have a lot
// of such fibers.
static const fiber_attr_t FIBER_ATTR_PTHREAD =
{ FIBER_STACKTYPE_PTHREAD, 0, NULL, FIBER_TAG_INVALID };

// fibers created with following attributes will have different size of
// stacks. Default is FIBER_ATTR_NORMAL.
static const fiber_attr_t FIBER_ATTR_SMALL = {FIBER_STACKTYPE_SMALL, 0, NULL,
                                                  FIBER_TAG_INVALID};
static const fiber_attr_t FIBER_ATTR_NORMAL = {FIBER_STACKTYPE_NORMAL, 0, NULL,
                                                   FIBER_TAG_INVALID};
static const fiber_attr_t FIBER_ATTR_LARGE = {FIBER_STACKTYPE_LARGE, 0, NULL,
                                                  FIBER_TAG_INVALID};

// fibers created with this attribute will print log when it's started,
// context-switched, finished.
static const fiber_attr_t FIBER_ATTR_DEBUG = {
    FIBER_STACKTYPE_NORMAL, FIBER_LOG_START_AND_FINISH | FIBER_LOG_CONTEXT_SWITCH, NULL,
    FIBER_TAG_INVALID};

static const size_t FIBER_EPOLL_THREAD_NUM = 1;
static const fiber_t FIBER_ATOMIC_INIT = 0;

// Min/Max number of work pthreads.
static const int FIBER_MIN_CONCURRENCY = 3 + FIBER_EPOLL_THREAD_NUM;
static const int FIBER_MAX_CONCURRENCY = 1024;

typedef struct {
    void* impl;
    // following fields are part of previous impl. and not used right now.
    // Don't remove them to break ABI compatibility.
    unsigned head;
    unsigned size;
    unsigned conflict_head;
    unsigned conflict_size;
} fiber_list_t;

// TODO: fiber_contention_site_t should be put into butex.
typedef struct {
    int64_t duration_ns;
    size_t sampling_range;
} fiber_contention_site_t;

typedef struct {
    unsigned* butex;
    fiber_contention_site_t csite;
} fiber_mutex_t;

typedef struct {
} fiber_mutexattr_t;

typedef struct {
    fiber_mutex_t* m;
    int* seq;
} fiber_cond_t;

typedef struct {
} fiber_condattr_t;

typedef struct {
} fiber_rwlock_t;

typedef struct {
} fiber_rwlockattr_t;

typedef struct {
    unsigned int count;
} fiber_barrier_t;

typedef struct {
} fiber_barrierattr_t;

typedef struct {
    uint64_t value;
} fiber_session_t;

// fiber_session returned by fiber_session_create* can never be this value.
// NOTE: don't confuse with INVALID_FIBER!
static const fiber_session_t INVALID_FIBER_ID = {0};

#if defined(__cplusplus)
// Overload operators for fiber_session_t
inline bool operator==(fiber_session_t id1, fiber_session_t id2)
{ return id1.value == id2.value; }
inline bool operator!=(fiber_session_t id1, fiber_session_t id2)
{ return !(id1 == id2); }
inline bool operator<(fiber_session_t id1, fiber_session_t id2)
{ return id1.value < id2.value; }
inline bool operator>(fiber_session_t id1, fiber_session_t id2)
{ return id2 < id1; }
inline bool operator<=(fiber_session_t id1, fiber_session_t id2)
{ return !(id2 < id1); }
inline bool operator>=(fiber_session_t id1, fiber_session_t id2)
{ return !(id1 < id2); }
inline std::ostream& operator<<(std::ostream& os, fiber_session_t id)
{ return os << id.value; }
#endif  // __cplusplus

typedef struct {
    void* impl;
    // following fields are part of previous impl. and not used right now.
    // Don't remove them to break ABI compatibility.
    unsigned head;
    unsigned size;
    unsigned conflict_head;
    unsigned conflict_size;
} fiber_session_list_t;

typedef uint64_t fiber_timer_t;

#endif  // MELON_FIBER_TYPES_H_
