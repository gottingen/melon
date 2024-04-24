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
#include <vector>
#include <google/protobuf/descriptor.h>

#include "melon/rpc/closure_guard.h"        // ClosureGuard
#include "melon/rpc/controller.h"           // Controller
#include "melon/builtin/common.h"
#include "melon/rpc/server.h"
#include "melon/proto/rpc/errno.pb.h"
#include "melon/builtin/bad_method_service.h"
#include "melon/rpc/details/server_private_accessor.h"


namespace melon {

void BadMethodService::no_method(::google::protobuf::RpcController* cntl_base,
                                 const BadMethodRequest* request,
                                 BadMethodResponse*,
                                 ::google::protobuf::Closure* done) {
    ClosureGuard done_guard(done);
    Controller *cntl = static_cast<Controller*>(cntl_base);
    const Server* server = cntl->server();
    const bool use_html = UseHTML(cntl->http_request());
    const char* newline = (use_html ? "<br>\n" : "\n");
    cntl->http_response().set_content_type(
        use_html ? "text/html" : "text/plain");

    std::ostringstream os;
    os << "Missing method name for service=" << request->service_name() << '.';
    const Server::ServiceProperty* sp = ServerPrivateAccessor(server)
        .FindServicePropertyAdaptively(request->service_name());
    if (sp != NULL && sp->service != NULL) {
        const google::protobuf::ServiceDescriptor* sd =
            sp->service->GetDescriptor();
        os << " Available methods are: " << newline << newline;
        for (int i = 0; i < sd->method_count(); ++i) {
            os << "rpc " << sd->method(i)->name()
               << " (" << sd->method(i)->input_type()->name()
               << ") returns (" << sd->method(i)->output_type()->name()
               << ");" << newline;
        }
    }
    if (sp != NULL && sp->restful_map != NULL) {
        os << " This path is associated with a RestfulMap!";
    }
    cntl->SetFailed(ENOMETHOD, "%s", os.str().c_str());
}

} // namespace melon
