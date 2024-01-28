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


#include "melon/butil/time.h"
#include "melon/butil/logging.h"
#include "melon/butil/popen.h"
#include "melon/rpc/controller.h"           // Controller
#include "melon/rpc/closure_guard.h"        // ClosureGuard
#include "melon/builtin/threads_service.h"
#include "melon/builtin/common.h"
#include "melon/butil/string_printf.h"

namespace melon {

    void ThreadsService::default_method(::google::protobuf::RpcController *cntl_base,
                                        const ::melon::ThreadsRequest *,
                                        ::melon::ThreadsResponse *,
                                        ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        cntl->http_response().set_content_type("text/plain");
        butil::IOBuf &resp = cntl->response_attachment();

        std::string cmd = butil::string_printf("pstack %lld", (long long) getpid());
        butil::Timer tm;
        tm.start();
        butil::IOBufBuilder pstack_output;
        const int rc = butil::read_command_output(pstack_output, cmd.c_str());
        if (rc < 0) {
            LOG(ERROR) << "Fail to popen `" << cmd << "'";
            return;
        }
        pstack_output.move_to(resp);
        tm.stop();
        resp.append(butil::string_printf("\n\ntime=%" PRId64 "ms", tm.m_elapsed()));
    }

} // namespace melon
