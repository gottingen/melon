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



#include <google/protobuf/descriptor.h>     // ServiceDescriptor
#include <melon/rpc/controller.h>           // Controller
#include <melon/rpc/server.h>               // Server
#include <melon/rpc/closure_guard.h>        // ClosureGuard
#include <melon/rpc/details/method_status.h>// MethodStatus
#include <melon/builtin/protobufs_service.h>
#include <melon/builtin/common.h>


namespace melon {

    ProtobufsService::ProtobufsService(Server *server) : _server(server) {
        MCHECK_EQ(0, Init());
    }

    int ProtobufsService::Init() {
        Server::ServiceMap &services = _server->_fullname_service_map;
        std::vector<const google::protobuf::Descriptor *> stack;
        stack.reserve(services.size() * 3);
        for (Server::ServiceMap::iterator
                     iter = services.begin(); iter != services.end(); ++iter) {
            if (!iter->second.is_user_service()) {
                continue;
            }
            const google::protobuf::ServiceDescriptor *d =
                    iter->second.service->GetDescriptor();
            _map[d->full_name()] = d->DebugString();
            const int method_count = d->method_count();
            for (int j = 0; j < method_count; ++j) {
                const google::protobuf::MethodDescriptor *md = d->method(j);
                stack.push_back(md->input_type());
                stack.push_back(md->output_type());
            }
        }
        while (!stack.empty()) {
            const google::protobuf::Descriptor *d = stack.back();
            stack.pop_back();
            _map[d->full_name()] = d->DebugString();
            for (int i = 0; i < d->field_count(); ++i) {
                const google::protobuf::FieldDescriptor *f = d->field(i);
                if (f->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE ||
                    f->type() == google::protobuf::FieldDescriptor::TYPE_GROUP) {
                    const google::protobuf::Descriptor *sub_d = f->message_type();
                    if (sub_d != d && _map.find(sub_d->full_name()) == _map.end()) {
                        stack.push_back(sub_d);
                    }
                }
            }
        }
        return 0;
    }

    void ProtobufsService::default_method(::google::protobuf::RpcController *cntl_base,
                                          const ProtobufsRequest *,
                                          ProtobufsResponse *,
                                          ::google::protobuf::Closure *done) {
        ClosureGuard done_guard(done);
        Controller *cntl = static_cast<Controller *>(cntl_base);
        mutil::IOBufBuilder os;
        const std::string &filter = cntl->http_request().unresolved_path();
        if (filter.empty()) {
            const bool use_html = UseHTML(cntl->http_request());
            cntl->http_response().set_content_type(
                    use_html ? "text/html" : "text/plain");
            if (use_html) {
                os << "<!DOCTYPE html><html><head></head><body>\n";
            }
            // list all structures.
            for (Map::iterator it = _map.begin(); it != _map.end(); ++it) {
                if (use_html) {
                    os << "<p><a href=\"/protobufs/" << it->first << "\">";
                }
                os << it->first;
                if (use_html) {
                    os << "</a></p>";
                }
                os << '\n';
            }
            if (use_html) {
                os << "</body></html>";
            }
        } else {
            // already text.
            cntl->http_response().set_content_type("text/plain");
            Map::iterator it = _map.find(filter);
            if (it == _map.end()) {
                cntl->SetFailed(ENOMETHOD,
                                "Fail to find any protobuf message by `%s'",
                                filter.c_str());
                return;
            }
            os << it->second;
        }
        os.move_to(cntl->response_attachment());
    }

} // namespace melon
