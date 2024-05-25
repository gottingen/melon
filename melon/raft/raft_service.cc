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

#include <melon/utility/logging.h>
#include <melon/rpc/server.h>
#include <melon/raft/raft_service.h>
#include <melon/raft/raft.h>
#include <melon/raft/node.h>
#include <melon/raft/node_manager.h>

namespace melon::raft {

    RaftServiceImpl::~RaftServiceImpl() {
        global_node_manager->remove_address(_addr);
    }

    void RaftServiceImpl::pre_vote(google::protobuf::RpcController *cntl_base,
                                   const RequestVoteRequest *request,
                                   RequestVoteResponse *response,
                                   google::protobuf::Closure *done) {
        melon::ClosureGuard done_guard(done);
        melon::Controller *cntl =
                static_cast<melon::Controller *>(cntl_base);

        PeerId peer_id;
        if (0 != peer_id.parse(request->peer_id())) {
            cntl->SetFailed(EINVAL, "peer_id invalid");
            return;
        }

        scoped_refptr<NodeImpl> node_ptr =
                global_node_manager->get(request->group_id(), peer_id);
        NodeImpl *node = node_ptr.get();
        if (!node) {
            cntl->SetFailed(ENOENT, "peer_id not exist");
            return;
        }

        // TODO: should return mutil::Status
        int rc = node->handle_pre_vote_request(request, response);
        if (rc != 0) {
            cntl->SetFailed(rc, "%s", berror(rc));
            return;
        }
    }

    void RaftServiceImpl::request_vote(google::protobuf::RpcController *cntl_base,
                                       const RequestVoteRequest *request,
                                       RequestVoteResponse *response,
                                       google::protobuf::Closure *done) {
        melon::ClosureGuard done_guard(done);
        melon::Controller *cntl =
                static_cast<melon::Controller *>(cntl_base);

        PeerId peer_id;
        if (0 != peer_id.parse(request->peer_id())) {
            cntl->SetFailed(EINVAL, "peer_id invalid");
            return;
        }

        scoped_refptr<NodeImpl> node_ptr =
                global_node_manager->get(request->group_id(), peer_id);
        NodeImpl *node = node_ptr.get();
        if (!node) {
            cntl->SetFailed(ENOENT, "peer_id not exist");
            return;
        }

        int rc = node->handle_request_vote_request(request, response);
        if (rc != 0) {
            cntl->SetFailed(rc, "%s", berror(rc));
            return;
        }
    }

    void RaftServiceImpl::append_entries(google::protobuf::RpcController *cntl_base,
                                         const AppendEntriesRequest *request,
                                         AppendEntriesResponse *response,
                                         google::protobuf::Closure *done) {
        melon::ClosureGuard done_guard(done);
        melon::Controller *cntl =
                static_cast<melon::Controller *>(cntl_base);

        PeerId peer_id;
        if (0 != peer_id.parse(request->peer_id())) {
            cntl->SetFailed(EINVAL, "peer_id invalid");
            return;
        }

        scoped_refptr<NodeImpl> node_ptr =
                global_node_manager->get(request->group_id(), peer_id);
        NodeImpl *node = node_ptr.get();
        if (!node) {
            cntl->SetFailed(ENOENT, "peer_id not exist");
            return;
        }

        return node->handle_append_entries_request(cntl, request, response,
                                                   done_guard.release());
    }

    void RaftServiceImpl::install_snapshot(google::protobuf::RpcController *cntl_base,
                                           const InstallSnapshotRequest *request,
                                           InstallSnapshotResponse *response,
                                           google::protobuf::Closure *done) {
        melon::Controller *cntl =
                static_cast<melon::Controller *>(cntl_base);

        PeerId peer_id;
        if (0 != peer_id.parse(request->peer_id())) {
            cntl->SetFailed(EINVAL, "peer_id invalid");
            done->Run();
            return;
        }

        scoped_refptr<NodeImpl> node_ptr =
                global_node_manager->get(request->group_id(), peer_id);
        NodeImpl *node = node_ptr.get();
        if (!node) {
            cntl->SetFailed(ENOENT, "peer_id not exist");

            done->Run();
            return;
        }

        node->handle_install_snapshot_request(cntl, request, response, done);
    }

    void RaftServiceImpl::timeout_now(::google::protobuf::RpcController *controller,
                                      const ::melon::raft::TimeoutNowRequest *request,
                                      ::melon::raft::TimeoutNowResponse *response,
                                      ::google::protobuf::Closure *done) {
        melon::Controller *cntl =
                static_cast<melon::Controller *>(controller);

        PeerId peer_id;
        if (0 != peer_id.parse(request->peer_id())) {
            cntl->SetFailed(EINVAL, "peer_id invalid");
            done->Run();
            return;
        }

        scoped_refptr<NodeImpl> node_ptr =
                global_node_manager->get(request->group_id(), peer_id);
        NodeImpl *node = node_ptr.get();
        if (!node) {
            cntl->SetFailed(ENOENT, "peer_id not exist");
            done->Run();
            return;
        }

        node->handle_timeout_now_request(cntl, request, response, done);
    }

}
