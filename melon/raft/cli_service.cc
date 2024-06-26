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


#include <melon/raft/cli_service.h>

#include <melon/rpc/controller.h>       // melon::Controller
#include <melon/raft/node_manager.h>          // NodeManager
#include <melon/raft/closure_helper.h>        // NewCallback

namespace melon::raft {

    static void add_peer_returned(melon::Controller *cntl,
                                  const AddPeerRequest *request,
                                  AddPeerResponse *response,
                                  std::vector<PeerId> old_peers,
                                  scoped_refptr<NodeImpl> /*node*/,
                                  ::google::protobuf::Closure *done,
                                  const mutil::Status &st) {
        melon::ClosureGuard done_guard(done);
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
            return;
        }
        bool already_exists = false;
        for (size_t i = 0; i < old_peers.size(); ++i) {
            response->add_old_peers(old_peers[i].to_string());
            response->add_new_peers(old_peers[i].to_string());
            if (old_peers[i] == request->peer_id()) {
                already_exists = true;
            }
        }
        if (!already_exists) {
            response->add_new_peers(request->peer_id());
        }
    }

    void CliServiceImpl::add_peer(::google::protobuf::RpcController *controller,
                                  const ::melon::raft::AddPeerRequest *request,
                                  ::melon::raft::AddPeerResponse *response,
                                  ::google::protobuf::Closure *done) {
        melon::Controller *cntl = (melon::Controller *) controller;
        melon::ClosureGuard done_guard(done);
        scoped_refptr<NodeImpl> node;
        mutil::Status st = get_node(&node, request->group_id(), request->leader_id());
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
            return;
        }
        std::vector<PeerId> peers;
        st = node->list_peers(&peers);
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
            return;
        }
        PeerId adding_peer;
        if (adding_peer.parse(request->peer_id()) != 0) {
            cntl->SetFailed(EINVAL, "Fail to parse peer_id %s",
                            request->peer_id().c_str());
            return;
        }
        LOG(WARNING) << "Receive AddPeerRequest to " << node->node_id()
                     << " from " << cntl->remote_side()
                     << ", adding " << request->peer_id();
        Closure *add_peer_done = NewCallback(
                add_peer_returned, cntl, request, response, peers, node,
                done_guard.release());
        return node->add_peer(adding_peer, add_peer_done);
    }

    static void remove_peer_returned(melon::Controller *cntl,
                                     const RemovePeerRequest *request,
                                     RemovePeerResponse *response,
                                     std::vector<PeerId> old_peers,
                                     scoped_refptr<NodeImpl> /*node*/,
                                     ::google::protobuf::Closure *done,
                                     const mutil::Status &st) {
        melon::ClosureGuard done_guard(done);
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
            return;
        }
        for (size_t i = 0; i < old_peers.size(); ++i) {
            response->add_old_peers(old_peers[i].to_string());
            if (old_peers[i] != request->peer_id()) {
                response->add_new_peers(old_peers[i].to_string());
            }
        }
    }

    void CliServiceImpl::remove_peer(::google::protobuf::RpcController *controller,
                                     const ::melon::raft::RemovePeerRequest *request,
                                     ::melon::raft::RemovePeerResponse *response,
                                     ::google::protobuf::Closure *done) {
        melon::Controller *cntl = (melon::Controller *) controller;
        melon::ClosureGuard done_guard(done);
        scoped_refptr<NodeImpl> node;
        mutil::Status st = get_node(&node, request->group_id(), request->leader_id());
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
            return;
        }
        std::vector<PeerId> peers;
        st = node->list_peers(&peers);
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
            return;
        }
        PeerId removing_peer;
        if (removing_peer.parse(request->peer_id()) != 0) {
            cntl->SetFailed(EINVAL, "Fail to parse peer_id %s",
                            request->peer_id().c_str());
            return;
        }
        LOG(WARNING) << "Receive RemovePeerRequest to " << node->node_id()
                     << " from " << cntl->remote_side()
                     << ", removing " << request->peer_id();
        Closure *remove_peer_done = NewCallback(
                remove_peer_returned, cntl, request, response, peers, node,
                done_guard.release());
        return node->remove_peer(removing_peer, remove_peer_done);
    }

    void CliServiceImpl::reset_peer(::google::protobuf::RpcController *controller,
                                    const ::melon::raft::ResetPeerRequest *request,
                                    ::melon::raft::ResetPeerResponse *response,
                                    ::google::protobuf::Closure *done) {
        melon::Controller *cntl = (melon::Controller *) controller;
        melon::ClosureGuard done_guard(done);
        scoped_refptr<NodeImpl> node;
        mutil::Status st = get_node(&node, request->group_id(), request->peer_id());
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
            return;
        }
        Configuration new_peers;
        for (int i = 0; i < request->new_peers_size(); ++i) {
            PeerId peer;
            if (peer.parse(request->new_peers(i)) != 0) {
                cntl->SetFailed(EINVAL, "Fail to parse %s",
                                request->new_peers(i).c_str());
                return;
            }
            new_peers.add_peer(peer);
        }
        LOG(WARNING) << "Receive set_peer to " << node->node_id()
                     << " from " << cntl->remote_side();
        st = node->reset_peers(new_peers);
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
        }
    }

    static void snapshot_returned(melon::Controller *cntl,
                                  scoped_refptr<NodeImpl> node,
                                  ::google::protobuf::Closure *done,
                                  const mutil::Status &st) {
        melon::ClosureGuard done_guard(done);
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
        }
    }

    void CliServiceImpl::snapshot(::google::protobuf::RpcController *controller,
                                  const ::melon::raft::SnapshotRequest *request,
                                  ::melon::raft::SnapshotResponse *response,
                                  ::google::protobuf::Closure *done) {

        melon::Controller *cntl = (melon::Controller *) controller;
        melon::ClosureGuard done_guard(done);
        scoped_refptr<NodeImpl> node;
        mutil::Status st = get_node(&node, request->group_id(), request->peer_id());
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
            return;
        }
        Closure *snapshot_done = NewCallback(snapshot_returned, cntl, node,
                                             done_guard.release());
        return node->snapshot(snapshot_done);
    }

    void CliServiceImpl::get_leader(::google::protobuf::RpcController *controller,
                                    const ::melon::raft::GetLeaderRequest *request,
                                    ::melon::raft::GetLeaderResponse *response,
                                    ::google::protobuf::Closure *done) {
        melon::Controller *cntl = (melon::Controller *) controller;
        melon::ClosureGuard done_guard(done);
        std::vector<scoped_refptr<NodeImpl> > nodes;
        if (request->has_peer_id()) {
            PeerId peer;
            if (peer.parse(request->peer_id()) != 0) {
                cntl->SetFailed(EINVAL, "Fail to parse %s",
                                request->peer_id().c_str());
                return;
            }
            scoped_refptr<NodeImpl> node =
                    global_node_manager->get(request->group_id(), peer);
            if (node) {
                nodes.push_back(node);
            }
        } else {
            global_node_manager->get_nodes_by_group_id(request->group_id(), &nodes);
        }
        if (nodes.empty()) {
            cntl->SetFailed(ENOENT, "No nodes in group %s",
                            request->group_id().c_str());
            return;
        }

        for (size_t i = 0; i < nodes.size(); ++i) {
            PeerId leader_id = nodes[i]->leader_id();
            if (!leader_id.is_empty()) {
                response->set_leader_id(leader_id.to_string());
                return;
            }
        }
        cntl->SetFailed(EAGAIN, "Unknown leader");
    }

    mutil::Status CliServiceImpl::get_node(scoped_refptr<NodeImpl> *node,
                                           const GroupId &group_id,
                                           const std::string &peer_id) {
        if (!peer_id.empty()) {
            *node = global_node_manager->get(group_id, peer_id);
            if (!(*node)) {
                return mutil::Status(ENOENT, "Fail to find node %s in group %s",
                                     peer_id.c_str(),
                                     group_id.c_str());
            }
        } else {
            std::vector<scoped_refptr<NodeImpl> > nodes;
            global_node_manager->get_nodes_by_group_id(group_id, &nodes);
            if (nodes.empty()) {
                return mutil::Status(ENOENT, "Fail to find node in group %s",
                                     group_id.c_str());
            }
            if (nodes.size() > 1) {
                return mutil::Status(EINVAL, "peer must be specified "
                                             "since there're %lu nodes in group %s",
                                     nodes.size(), group_id.c_str());
            }
            *node = nodes.front();
        }

        if ((*node)->disable_cli()) {
            return mutil::Status(EACCES, "CliService is not allowed to access node "
                                         "%s", (*node)->node_id().to_string().c_str());
        }

        return mutil::Status::OK();
    }

    static void change_peers_returned(melon::Controller *cntl,
                                      const ChangePeersRequest *request,
                                      ChangePeersResponse *response,
                                      std::vector<PeerId> old_peers,
                                      Configuration new_peers,
                                      scoped_refptr<NodeImpl> /*node*/,
                                      ::google::protobuf::Closure *done,
                                      const mutil::Status &st) {
        melon::ClosureGuard done_guard(done);
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
            return;
        }
        for (size_t i = 0; i < old_peers.size(); ++i) {
            response->add_old_peers(old_peers[i].to_string());
        }
        for (Configuration::const_iterator
                     iter = new_peers.begin(); iter != new_peers.end(); ++iter) {
            response->add_new_peers(iter->to_string());
        }
    }

    void CliServiceImpl::change_peers(::google::protobuf::RpcController *controller,
                                      const ::melon::raft::ChangePeersRequest *request,
                                      ::melon::raft::ChangePeersResponse *response,
                                      ::google::protobuf::Closure *done) {
        melon::Controller *cntl = (melon::Controller *) controller;
        melon::ClosureGuard done_guard(done);
        scoped_refptr<NodeImpl> node;
        mutil::Status st = get_node(&node, request->group_id(), request->leader_id());
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
            return;
        }
        std::vector<PeerId> old_peers;
        st = node->list_peers(&old_peers);
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
            return;
        }
        Configuration conf;
        for (int i = 0; i < request->new_peers_size(); ++i) {
            PeerId peer;
            if (peer.parse(request->new_peers(i)) != 0) {
                cntl->SetFailed(EINVAL, "Fail to parse %s",
                                request->new_peers(i).c_str());
                return;
            }
            conf.add_peer(peer);
        }
        Closure *change_peers_done = NewCallback(
                change_peers_returned,
                cntl, request, response, old_peers, conf, node,
                done_guard.release());
        return node->change_peers(conf, change_peers_done);
    }

    void CliServiceImpl::transfer_leader(
            ::google::protobuf::RpcController *controller,
            const ::melon::raft::TransferLeaderRequest *request,
            ::melon::raft::TransferLeaderResponse *response,
            ::google::protobuf::Closure *done) {
        melon::Controller *cntl = (melon::Controller *) controller;
        melon::ClosureGuard done_guard(done);
        scoped_refptr<NodeImpl> node;
        mutil::Status st = get_node(&node, request->group_id(), request->leader_id());
        if (!st.ok()) {
            cntl->SetFailed(st.error_code(), "%s", st.error_cstr());
            return;
        }
        PeerId peer = ANY_PEER;
        if (request->has_peer_id() && peer.parse(request->peer_id()) != 0) {
            cntl->SetFailed(EINVAL, "Fail to parse %s", request->peer_id().c_str());
            return;
        }
        const int rc = node->transfer_leadership_to(peer);
        if (rc != 0) {
            cntl->SetFailed(rc, "Fail to invoke transfer_leadership_to : %s",
                            berror(rc));
            return;
        }
    }

}  //  namespace melon::raft
