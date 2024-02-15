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


#include "melon/raft/builtin_service_impl.h"

#include <melon/rpc/controller.h>
#include <melon/rpc/closure_guard.h>
#include <melon/rpc/http/http_status_code.h>
#include <melon/builtin/common.h>
#include "melon/raft/node.h"
#include "melon/raft/replicator.h"
#include "melon/raft/node_manager.h"

namespace melon::raft {

    void RaftStatImpl::GetTabInfo(melon::TabInfoList *info_list) const {
        melon::TabInfo *info = info_list->add();
        info->tab_name = "raft";
        info->path = "/raft_stat";
    }

    void RaftStatImpl::default_method(::google::protobuf::RpcController *controller,
                                      const ::melon::raft::IndexRequest * /*request*/,
                                      ::melon::raft::IndexResponse * /*response*/,
                                      ::google::protobuf::Closure *done) {
        melon::ClosureGuard done_guard(done);
        melon::Controller *cntl = (melon::Controller *) controller;
        std::string group_id = cntl->http_request().unresolved_path();
        std::vector<scoped_refptr<NodeImpl> > nodes;
        if (group_id.empty()) {
            global_node_manager->get_all_nodes(&nodes);
        } else {
            global_node_manager->get_nodes_by_group_id(group_id, &nodes);
        }
        const bool html = melon::UseHTML(cntl->http_request());
        if (html) {
            cntl->http_response().set_content_type("text/html");
        } else {
            cntl->http_response().set_content_type("text/plain");
        }
        mutil::IOBufBuilder os;
        if (html) {
            os << "<!DOCTYPE html><html><head>\n"
               << "<script language=\"javascript\" type=\"text/javascript\" src=\"/js/jquery_min\"></script>\n"
               << melon::TabsHead() << "</head><body>";
            cntl->server()->PrintTabsBody(os, "raft");
        }
        if (nodes.empty()) {
            if (html) {
                os << "</body></html>";
            }
            os.move_to(cntl->response_attachment());
            return;
        }

        std::string prev_group_id;
        const char *newline = html ? "<br>" : "\r\n";
        for (size_t i = 0; i < nodes.size(); ++i) {
            const NodeId node_id = nodes[i]->node_id();
            group_id = node_id.group_id;
            if (group_id != prev_group_id) {
                if (html) {
                    os << "<h1>" << group_id << "</h1>";
                } else {
                    os << "[" << group_id << "]" << newline;
                }
                prev_group_id = group_id;
            }
            nodes[i]->describe(os, html);
            os << newline;
        }
        if (html) {
            os << "</body></html>";
        }
        os.move_to(cntl->response_attachment());
    }

}  //  namespace melon::raft
