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



#include <ostream>
#include <melon/rpc/closure_guard.h>        // ClosureGuard
#include <melon/rpc/controller.h>           // Controller
#include <melon/builtin/common.h>
#include <melon/builtin/fibers_service.h>

namespace fiber {
    void print_task(std::ostream &os, fiber_t tid);
}


namespace melon {

    void FibersService::default_method(::google::protobuf::RpcController *cntl_base,
                                         const ::melon::FibersRequest *,
                                         ::melon::FibersResponse *,
                                         ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        cntl->http_response().set_content_type("text/plain");
        mutil::IOBufBuilder os;
        const std::string &constraint = cntl->http_request().unresolved_path();

        if (constraint.empty()) {
            os << "Use /fibers/<fiber_session>";
        } else {
            char *endptr = NULL;
            fiber_t tid = strtoull(constraint.c_str(), &endptr, 10);
            if (*endptr == '\0' || *endptr == '/') {
                ::fiber::print_task(os, tid);
            } else {
                cntl->SetFailed(ENOMETHOD, "path=%s is not a fiber id",
                                constraint.c_str());
            }
        }
        os.move_to(cntl->response_attachment());
    }

} // namespace melon
