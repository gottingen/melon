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
