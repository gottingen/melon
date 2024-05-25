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
