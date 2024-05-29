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


#ifndef MELON_RPC_LOG_H_
#define MELON_RPC_LOG_H_

#include <inttypes.h>  // PRId64 PRIu64
#include <melon/fiber/errno.h>

#define RPC_VLOG_LEVEL     99
#define RPC_VLOG_IS_ON     VLOG_IS_ON(RPC_VLOG_LEVEL)
#define RPC_VLOG           VLOG(RPC_VLOG_LEVEL)
#define RPC_VPLOG          VPLOG(RPC_VLOG_LEVEL)
#define RPC_VLOG_IF(cond)  LOG_IF(INFO, cond).WithVerbosity(RPC_VLOG_LEVEL)
#define RPC_VPLOG_IF(cond) VPLOG_IF(RPC_VLOG_LEVEL, (cond))

#endif  // MELON_RPC_LOG_H_
