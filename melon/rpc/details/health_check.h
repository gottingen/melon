// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.


#ifndef _HEALTH_CHECK_H
#define _HEALTH_CHECK_H

#include "melon/rpc/socket_id.h"
#include "melon/rpc/periodic_task.h"
#include "melon/metrics/all.h"
#include "melon/rpc/socket.h"

namespace melon::rpc {

    // Start health check for socket id after delay_ms.
    // If delay_ms <= 0, HealthCheck would be started
    // immediately.
    void StartHealthCheck(SocketId id, int64_t delay_ms);

} // namespace melon::rpc

#endif
