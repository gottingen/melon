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

#define RPC_VMLOG_LEVEL     99
#define RPC_VMLOG_IS_ON     VMLOG_IS_ON(RPC_VMLOG_LEVEL)
#define RPC_VMLOG           VMLOG(RPC_VMLOG_LEVEL)
#define RPC_VPLOG          VPMLOG(RPC_VMLOG_LEVEL)
#define RPC_VMLOG_IF(cond)  VMLOG_IF(RPC_VMLOG_LEVEL, (cond))
#define RPC_VPLOG_IF(cond) VPMLOG_IF(RPC_VMLOG_LEVEL, (cond))

#endif  // MELON_RPC_LOG_H_
