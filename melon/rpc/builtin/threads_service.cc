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

#include <inttypes.h>
#include "melon/times/time.h"
#include "melon/log/logging.h"
#include "melon/system/process.h"
#include "melon/rpc/controller.h"           // Controller
#include "melon/rpc/closure_guard.h"        // ClosureGuard
#include "melon/rpc/builtin/threads_service.h"
#include "melon/rpc/builtin/common.h"
#include "melon/strings/str_format.h"

namespace melon::rpc {

    void ThreadsService::default_method(::google::protobuf::RpcController *cntl_base,
                                        const ::melon::rpc::ThreadsRequest *,
                                        ::melon::rpc::ThreadsResponse *,
                                        ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        cntl->http_response().set_content_type("text/plain");
        melon::cord_buf &resp = cntl->response_attachment();

        std::string cmd = melon::string_printf("pstack %lld", (long long) getpid());
        melon::stop_watcher tm;
        tm.start();
        melon::cord_buf_builder pstack_output;
        const int rc = melon::read_command_output(pstack_output, cmd.c_str());
        if (rc < 0) {
            MELON_LOG(ERROR) << "Fail to popen `" << cmd << "'";
            return;
        }
        pstack_output.move_to(resp);
        tm.stop();
        resp.append(melon::string_printf("\n\ntime=%" PRId64 "ms", tm.m_elapsed()));
    }

} // namespace melon::rpc
