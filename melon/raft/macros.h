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


#ifndef  BRAFT_MACROS_H
#define  BRAFT_MACROS_H

#include <melon/utility/macros.h>
#include <melon/utility/logging.h>
#include <melon/var/utils/lock_timer.h>

#define BRAFT_VMLOG_IS_ON     VMLOG_IS_ON(89)
#define BRAFT_VMLOG           VMLOG(89)
#define BRAFT_VPLOG          VPMLOG(89)
#define BRAFT_VMLOG_IF(cond)  VMLOG_IF(89, (cond))
#define BRAFT_VPLOG_IF(cond) VPMLOG_IF(89, (cond))

//#define USE_FIBER_MUTEX

#ifdef USE_FIBER_MUTEX

#include <melon/fiber/mutex.h>

namespace melon::raft {
typedef ::fiber::Mutex raft_mutex_t;
}  // namespace melon::raft

#else   // USE_FIBER_MUTEX

#include <melon/utility/synchronization/lock.h>
namespace melon::raft {
typedef ::mutil::Mutex raft_mutex_t;
}  // namespace melon::raft

#endif  // USE_FIBER_MUTEX

#ifdef UNIT_TEST
#define BRAFT_MOCK virtual
#else
#define BRAFT_MOCK
#endif

#define BRAFT_GET_ARG3(arg1, arg2, arg3, ...)  arg3

#define BRAFT_RETURN_IF1(expr, rc)       \
    do {                                \
        if ((expr)) {                   \
            return (rc);                \
        }                               \
    } while (0)

#define BRAFT_RETURN_IF0(expr)           \
    do {                                \
        if ((expr)) {                   \
            return;                     \
        }                               \
    } while (0)

#define BRAFT_RETURN_IF(expr, args...)   \
        BRAFT_GET_ARG3(1, ##args, BRAFT_RETURN_IF1, BRAFT_RETURN_IF0)(expr, ##args)

#endif  //BRAFT_MACROS_H