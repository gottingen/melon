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


#ifndef  BRAFT_MACROS_H
#define  BRAFT_MACROS_H

#include <melon/utility/macros.h>
#include <turbo/log/logging.h>
#include <melon/var/utils/lock_timer.h>

#define BRAFT_VLOG_IS_ON     VLOG_IS_ON(89)
#define BRAFT_VLOG           VLOG(89)
#define BRAFT_VPLOG          VPLOG(89)
#define BRAFT_VLOG_IF(cond)  LOG_IF(INFO, cond).WithVerbosity(89)
#define BRAFT_VPLOG_IF(cond) VPLOG_IF(89, (cond))

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
