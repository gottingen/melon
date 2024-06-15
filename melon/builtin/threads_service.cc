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



#include <melon/utility/time.h>
#include <turbo/log/logging.h>
#include <melon/base/popen.h>
#include <melon/rpc/controller.h>           // Controller
#include <melon/rpc/closure_guard.h>        // ClosureGuard
#include <melon/builtin/threads_service.h>
#include <melon/builtin/common.h>
#include <melon/utility/string_printf.h>
#include <cinttypes>

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
            LOG(ERROR) << "Fail to popen `" << cmd << "'";
            return;
        }
        pstack_output.move_to(resp);
        tm.stop();
        resp.append(mutil::string_printf("\n\ntime=%" PRId64 "ms", tm.m_elapsed()));
    }

} // namespace melon
