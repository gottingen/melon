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
#include <vector>
#include <google/protobuf/descriptor.h>

#include <melon/rpc/closure_guard.h>        // ClosureGuard
#include <melon/rpc/controller.h>           // Controller
#include <melon/builtin/common.h>
#include <melon/rpc/server.h>
#include <melon/proto/rpc/errno.pb.h>
#include <melon/builtin/bad_method_service.h>
#include <melon/rpc/details/server_private_accessor.h>


namespace melon {

    void BadMethodService::no_method(::google::protobuf::RpcController *cntl_base,
                                     const BadMethodRequest *request,
                                     BadMethodResponse *,
                                     ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        auto *cntl = static_cast<Controller *>(cntl_base);
        const Server *server = cntl->server();
        const bool use_html = UseHTML(cntl->http_request());
        const char *newline = (use_html ? "<br>\n" : "\n");
        cntl->http_response().set_content_type(
                use_html ? "text/html" : "text/plain");

        std::ostringstream os;
        os << "Missing method name for service=" << request->service_name() << '.';
        const Server::ServiceProperty *sp = ServerPrivateAccessor(server)
                .FindServicePropertyAdaptively(request->service_name());
        if (sp != nullptr && sp->service != nullptr) {
            const google::protobuf::ServiceDescriptor *sd =
                    sp->service->GetDescriptor();
            os << " Available methods are: " << newline << newline;
            for (int i = 0; i < sd->method_count(); ++i) {
                os << "rpc " << sd->method(i)->name()
                   << " (" << sd->method(i)->input_type()->name()
                   << ") returns (" << sd->method(i)->output_type()->name()
                   << ");" << newline;
            }
        }
        if (sp != nullptr && sp->restful_map != nullptr) {
            os << " This path is associated with a RestfulMap!";
        }
        cntl->SetFailed(ENOMETHOD, "%s", os.str().c_str());
    }

} // namespace melon
