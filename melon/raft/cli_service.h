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

#pragma once

#include <melon/utility/status.h>
#include <melon/proto/raft/cli.pb.h>                // CliService
#include <melon/raft/node.h>                  // NodeImpl

namespace melon::raft {

    class CliServiceImpl : public CliService {
    public:
        void add_peer(::google::protobuf::RpcController *controller,
                      const ::melon::raft::AddPeerRequest *request,
                      ::melon::raft::AddPeerResponse *response,
                      ::google::protobuf::Closure *done);

        void remove_peer(::google::protobuf::RpcController *controller,
                         const ::melon::raft::RemovePeerRequest *request,
                         ::melon::raft::RemovePeerResponse *response,
                         ::google::protobuf::Closure *done);

        void reset_peer(::google::protobuf::RpcController *controller,
                        const ::melon::raft::ResetPeerRequest *request,
                        ::melon::raft::ResetPeerResponse *response,
                        ::google::protobuf::Closure *done);

        void snapshot(::google::protobuf::RpcController *controller,
                      const ::melon::raft::SnapshotRequest *request,
                      ::melon::raft::SnapshotResponse *response,
                      ::google::protobuf::Closure *done);

        void get_leader(::google::protobuf::RpcController *controller,
                        const ::melon::raft::GetLeaderRequest *request,
                        ::melon::raft::GetLeaderResponse *response,
                        ::google::protobuf::Closure *done);

        void change_peers(::google::protobuf::RpcController *controller,
                          const ::melon::raft::ChangePeersRequest *request,
                          ::melon::raft::ChangePeersResponse *response,
                          ::google::protobuf::Closure *done);

        void transfer_leader(::google::protobuf::RpcController *controller,
                             const ::melon::raft::TransferLeaderRequest *request,
                             ::melon::raft::TransferLeaderResponse *response,
                             ::google::protobuf::Closure *done);

    private:
        mutil::Status get_node(scoped_refptr<NodeImpl> *node,
                               const GroupId &group_id,
                               const std::string &peer_id);
    };

}  // namespace melon::raft
