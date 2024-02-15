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


#ifndef MELON_RPC_LOG_H_
#define MELON_RPC_LOG_H_

#include <inttypes.h>  // PRId64 PRIu64
#include "melon/fiber/errno.h"

#define RPC_VLOG_LEVEL     99
#define RPC_VLOG_IS_ON     VLOG_IS_ON(RPC_VLOG_LEVEL)
#define RPC_VLOG           VLOG(RPC_VLOG_LEVEL)
#define RPC_VPLOG          VPLOG(RPC_VLOG_LEVEL)
#define RPC_VLOG_IF(cond)  VLOG_IF(RPC_VLOG_LEVEL, (cond))
#define RPC_VPLOG_IF(cond) VPLOG_IF(RPC_VLOG_LEVEL, (cond))

#endif  // MELON_RPC_LOG_H_
