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



#include "melon/utility/time.h"
#include "melon/utility/logging.h"
#include "melon/utility/popen.h"
#include "melon/rpc/controller.h"           // Controller
#include "melon/rpc/closure_guard.h"        // ClosureGuard
#include "melon/builtin/threads_service.h"
#include "melon/builtin/common.h"
#include "melon/utility/string_printf.h"

namespace melon {

    void ThreadsService::default_method(::google::protobuf::RpcController *cntl_base,
                                        const ::melon::ThreadsRequest *,
                                        ::melon::ThreadsResponse *,
                                        ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        cntl->http_response().set_content_type("text/plain");
        mutil::IOBuf &resp = cntl->response_attachment();

        std::string cmd = mutil::string_printf("pstack %lld", (long long) getpid());
        mutil::Timer tm;
        tm.start();
        mutil::IOBufBuilder pstack_output;
        const int rc = mutil::read_command_output(pstack_output, cmd.c_str());
        if (rc < 0) {
            MLOG(ERROR) << "Fail to popen `" << cmd << "'";
            return;
        }
        pstack_output.move_to(resp);
        tm.stop();
        resp.append(mutil::string_printf("\n\ntime=%" PRId64 "ms", tm.m_elapsed()));
    }

} // namespace melon
