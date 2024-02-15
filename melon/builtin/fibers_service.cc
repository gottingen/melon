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



#include <ostream>
#include "melon/rpc/closure_guard.h"        // ClosureGuard
#include "melon/rpc/controller.h"           // Controller
#include "melon/builtin/common.h"
#include "melon/builtin/fibers_service.h"

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
